#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// gcc -o simulador_cache.exe simulador_cache.c; echo "0`n128`n16`n4`n5`n1`n70`n70`nC:\resultado_teste.txt" | .\simulador_cache.exe

#define TAMANHO_INT 65535
#define TAMANHO_END 32

#define INPUT_INVALIDO 1
#define ARQUIVO_INVALIDO 2

typedef struct str_dados_entrada
{
    int politica_escrita;
    int tamanho_linha;
    int numero_linhas;
    int associatividade;
    int hit_time;
    int politica_subs;
    int tempo_mp_leitura;
    int tempo_mp_escrita;
    int arquivo_input;
} str_dados_entrada;

// ALTERAÇÃO: Estrutura da linha completamente reformulada
typedef struct linha_cache
{
    int valid;      // ALTERAÇÃO: Adicionado bit de validade
    int tag;        // ALTERAÇÃO: Adicionado tag
    int dirty;      // ALTERAÇÃO: Dirty bit movido para a linha
    long long int lru_counter; // ALTERAÇÃO: Contador LRU individual por linha
} str_linha_cache;

// ALTERAÇÃO: Simplificada estrutura do conjunto
typedef struct conjunto_cache
{
    str_linha_cache *linhas;  // ALTERAÇÃO: Array de linhas simplificado
} str_conjunto_cache;

typedef struct informacoes_cache
{
    int numero_conjuntos;
    int mascara_conjuntos;
    // ALTERAÇÃO: Removido index_ocupacao_cache (não necessário)

    struct
    {
        int rotulo;
        int conjunto;
        int palavra;
    }endereco;
}str_informacoes_cache;

struct estatisticas
{
    int total_acessos;
    int total_leituras;
    int total_escritas;
    int hits_leitura;
    int hits_escrita;
    int leitura_mp;
    int escrita_mp;

    double tempo_total_acesso;

    int quant_enderecos;
    int acessos_mp;
    int hit_rate;
    int hit_rate_leituras;
    int hit_rate_escritas;
    int tempo_medio_cache;
} stats;

long long int contador_lru;
char caminho_saida[100];

str_dados_entrada     dados_entrada;
str_informacoes_cache informacoes_cache;
str_conjunto_cache*   conjuntos_cache;

void trata_erro(int erro)
{
    printf("ERRO!");

    switch (erro)
    {
    case 1:
        printf("Valor inserido esta fora dos limites! \n");
        break;
    case 2:
        printf("O Arquivo eh invalido! \n");
        break;
    default:
        printf("Erro Fatal! \n");
    }

    printf("Finalizando testes... \n");

    exit(erro);
}

// ALTERAÇÃO: Funções auxiliares para extrair campos do endereço
int extrai_offset(int endereco) {
    return endereco & ((1 << informacoes_cache.endereco.palavra) - 1);
}

int extrai_indice_conjunto(int endereco) {
    return (endereco >> informacoes_cache.endereco.palavra) & ((1 << informacoes_cache.endereco.conjunto) - 1);
}

int extrai_tag(int endereco) {
    return endereco >> (informacoes_cache.endereco.palavra + informacoes_cache.endereco.conjunto);
}

void inicializa_cache()
{
    int i, j;
    // Calcula a quantidade de conjuntos
    informacoes_cache.numero_conjuntos = dados_entrada.numero_linhas / dados_entrada.associatividade;

    // Formata o endereco de acordo com os inputs
    informacoes_cache.endereco.palavra = log2(dados_entrada.tamanho_linha);
    informacoes_cache.endereco.conjunto = log2(informacoes_cache.numero_conjuntos);
    informacoes_cache.endereco.rotulo = TAMANHO_END - (informacoes_cache.endereco.palavra + informacoes_cache.endereco.conjunto);

    // ALTERAÇÃO: Máscara corrigida (não é mais necessária com as novas funções)
    informacoes_cache.mascara_conjuntos = (1 << informacoes_cache.endereco.conjunto) - 1;

    printf("Rotulo: %d\n", informacoes_cache.endereco.rotulo);
    printf("Conjunto: %d\n", informacoes_cache.endereco.conjunto);
    printf("Palavra: %d\n", informacoes_cache.endereco.palavra);

    // Aloca a quantidade de conjuntos da cache
    conjuntos_cache = (str_conjunto_cache*) malloc (informacoes_cache.numero_conjuntos * sizeof(str_conjunto_cache));

    // ALTERAÇÃO: Inicialização corrigida dos conjuntos e linhas
    for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
    {
        conjuntos_cache[i].linhas = (str_linha_cache*) malloc (dados_entrada.associatividade * sizeof(str_linha_cache));
        
        // Inicializa todas as linhas como inválidas
        for(j = 0; j < dados_entrada.associatividade; j++)
        {
            conjuntos_cache[i].linhas[j].valid = 0;    // ALTERAÇÃO: Linha inválida
            conjuntos_cache[i].linhas[j].tag = 0;      // ALTERAÇÃO: Tag zerado
            conjuntos_cache[i].linhas[j].dirty = 0;    // ALTERAÇÃO: Não modificado
            conjuntos_cache[i].linhas[j].lru_counter = 0; // ALTERAÇÃO: Contador LRU
        }
    }

    printf("Inicializou a cache!\n");
    printf("Numero de Linhas: %d\n", dados_entrada.numero_linhas);
    printf("Linhas por conjunto: %d\n", dados_entrada.associatividade);
    printf("Numero de conjuntos: %d\n", informacoes_cache.numero_conjuntos);

    // Inicializar estatísticas
    stats.total_acessos = 0;
    stats.total_leituras = 0;
    stats.total_escritas = 0;
    stats.hits_leitura = 0;
    stats.hits_escrita = 0;
    stats.leitura_mp = 0;
    stats.escrita_mp = 0;
    stats.tempo_total_acesso = 0.0;
    stats.quant_enderecos = 0;
    stats.acessos_mp = 0;
    stats.hit_rate = 0;
    stats.tempo_medio_cache = 0;
    contador_lru = 0;
}

// ALTERAÇÃO: Função busca_lru completamente reescrita
int busca_linha_lru(int indice_conjunto)
{
    int i, linha_lru = 0;
    long long int menor_contador = conjuntos_cache[indice_conjunto].linhas[0].lru_counter;

    // Busca a linha com menor contador LRU no conjunto específico
    for(i = 1; i < dados_entrada.associatividade; i++)
    {
        if(conjuntos_cache[indice_conjunto].linhas[i].lru_counter < menor_contador)
        {
            menor_contador = conjuntos_cache[indice_conjunto].linhas[i].lru_counter;
            linha_lru = i;
        }
    }

    return linha_lru;
}

// ALTERAÇÃO: Função busca_conjunto_mp removida - não é mais necessária

// ALTERAÇÃO: Nova função para processar acesso à cache
void processa_acesso_cache(int endereco, char operacao)
{
    int tag = extrai_tag(endereco);
    int indice_conjunto = extrai_indice_conjunto(endereco);
    int i, hit = 0, linha_hit = -1;
    
    // Busca pela tag no conjunto correto
    for(i = 0; i < dados_entrada.associatividade; i++)
    {
        if(conjuntos_cache[indice_conjunto].linhas[i].valid && 
           conjuntos_cache[indice_conjunto].linhas[i].tag == tag)
        {
            hit = 1;
            linha_hit = i;
            break;
        }
    }
    
    if(hit)
    {
        // HIT - atualiza contador LRU
        contador_lru++;
        conjuntos_cache[indice_conjunto].linhas[linha_hit].lru_counter = contador_lru;
        
        if(operacao == 'R')
        {
            stats.hits_leitura++;
        }
        else if(operacao == 'W')
        {
            stats.hits_escrita++;
            
            // ALTERAÇÃO: Marca como dirty se for write-back
            if(dados_entrada.politica_escrita == 1)
            {
                conjuntos_cache[indice_conjunto].linhas[linha_hit].dirty = 1;
            }
            else
            {
                // Write-through: escreve imediatamente na MP
                stats.escrita_mp++;
            }
        }
    }
    else
    {
        // MISS - precisa buscar da memória principal
        stats.leitura_mp++;
        
        // Encontra linha para substituir
        int linha_substituir = -1;
        
        // Primeiro, procura linha inválida
        for(i = 0; i < dados_entrada.associatividade; i++)
        {
            if(!conjuntos_cache[indice_conjunto].linhas[i].valid)
            {
                linha_substituir = i;
                break;
            }
        }
        
        // Se não há linha inválida, aplica política de substituição
        if(linha_substituir == -1)
        {
            if(dados_entrada.politica_subs == 0) // LRU
            {
                linha_substituir = busca_linha_lru(indice_conjunto);
            }
            else // Aleatória
            {
                linha_substituir = rand() % dados_entrada.associatividade;
            }
            
            // Se a linha a ser substituída está dirty, escreve na MP
            if(conjuntos_cache[indice_conjunto].linhas[linha_substituir].dirty)
            {
                stats.escrita_mp++;
            }
        }
        
        // Atualiza a linha com novo conteúdo
        conjuntos_cache[indice_conjunto].linhas[linha_substituir].valid = 1;
        conjuntos_cache[indice_conjunto].linhas[linha_substituir].tag = tag;
        conjuntos_cache[indice_conjunto].linhas[linha_substituir].dirty = 0;
        contador_lru++;
        conjuntos_cache[indice_conjunto].linhas[linha_substituir].lru_counter = contador_lru;
        
        // Se é escrita
        if(operacao == 'W')
        {
            if(dados_entrada.politica_escrita == 1) // Write-back
            {
                conjuntos_cache[indice_conjunto].linhas[linha_substituir].dirty = 1;
            }
            else // Write-through
            {
                stats.escrita_mp++;
            }
        }
    }
}

// ALTERAÇÃO: Função finaliza_write_back reescrita
void finaliza_write_back(void)
{
    int i, j;
    
    // Passa por todos os conjuntos e todas as linhas
    for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
    {
        for(j = 0; j < dados_entrada.associatividade; j++)
        {
            // Se a linha é válida e está dirty, escreve na MP
            if(conjuntos_cache[i].linhas[j].valid && 
               conjuntos_cache[i].linhas[j].dirty)
            {
                stats.escrita_mp++;
            }
        }
    }
}

void trata_dados_entrada()
{
    printf("--------------------------------------------------------------------------------------------------------------------\n");
    printf("|                                                                                                                  |\n");
    printf("|                          Insira os dados sobre as memorias para prosseguir com o programa                        |\n");
    printf("|                                                                                                                  |\n");
    printf("--------------------------------------------------------------------------------------------------------------------\n");

    printf("-------------------------------------------------------------------------\n");
    printf("|                                                                       |\n");
    printf("|                          Dados da Cache                               |\n");
    printf("|                                                                       |\n");
    printf("-------------------------------------------------------------------------\n");

    // Politica de Escrita
    printf("Insira a politica de escrita \n");
    printf("(0 - Write Thorugh, 1 - Write-Back) \n");
    scanf("%i", &dados_entrada.politica_escrita);
    if (dados_entrada.politica_escrita > 1 || dados_entrada.politica_escrita < 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Tamanho Linha
    printf("Insira o tamanho da linha \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.tamanho_linha);
    // ALTERAÇÃO: Correção na verificação de potência de 2
    if ((dados_entrada.tamanho_linha & (dados_entrada.tamanho_linha - 1)) != 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de Linhas
    printf("Insira o numero de linhas \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.numero_linhas);
    // ALTERAÇÃO: Correção na verificação de potência de 2
    if ((dados_entrada.numero_linhas & (dados_entrada.numero_linhas - 1)) != 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de linhas por conjunto
    printf("Insira o numero de linhas por conjunto \n");
    printf("(Deve ser maior que 1 e menor que o total de linhas) \n");
    scanf("%i", &dados_entrada.associatividade);
    if (dados_entrada.associatividade < 1 || dados_entrada.associatividade > dados_entrada.numero_linhas)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Hit Time
    printf("Insira o tempo de acesso da memoria \n");
    printf("(Tempo em Nanossegundos) \n");
    scanf("%i", &dados_entrada.hit_time);
    if (dados_entrada.hit_time <= 0 || dados_entrada.hit_time > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Politica de substituicao
    printf("Insira a politica de substituicao \n");
    printf("(0 - LRU, 1 - Aleatoria) \n");
    scanf("%i", &dados_entrada.politica_subs);
    if (dados_entrada.politica_subs > 1 || dados_entrada.politica_subs < 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    printf("-------------------------------------------------------------------------\n");
    printf("|                                                                       |\n");
    printf("|                     Dados da Memoria Principal                        |\n");
    printf("|                                                                       |\n");
    printf("-------------------------------------------------------------------------\n");
    // Tempo de Leitura
    printf("Insira o tempo de leitura da Memoria Principal \n");
    printf("(Tempo em Nanossegundos) \n");
    scanf("%i", &dados_entrada.tempo_mp_leitura);
    if (dados_entrada.tempo_mp_leitura <= 0 || dados_entrada.tempo_mp_leitura > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Tempo de escrita
    printf("Insira o tempo de escrita da Memoria Principal \n");
    printf("(Tempo em Nanossegundos) \n");
    scanf("%i", &dados_entrada.tempo_mp_escrita);
    if (dados_entrada.tempo_mp_escrita <= 0 || dados_entrada.tempo_mp_escrita > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    printf("-------------------------------------------------------------------------\n");
    printf("|                                                                       |\n");
    printf("|                         Arquivo dos Dados                             |\n");
    printf("|                                                                       |\n");
    printf("-------------------------------------------------------------------------\n");

    printf("Insira qual o arquivo de dados desejado: \n");
    printf("(0 - Teste, 1 - Oficial) \n");
    scanf("%i", &dados_entrada.arquivo_input);

    printf("Insira o caminho do arquivo de saida \n");
    scanf("%s", caminho_saida);
}

// ALTERAÇÃO: Função cria_arquivo_saida com cálculos corrigidos
void cria_arquivo_saida()
{
    FILE *arquivo_saida;
    arquivo_saida = fopen(caminho_saida, "w+");

    if (arquivo_saida == NULL)
    {
        printf("Erro ao criar arquivo de saida!\n");
        return;
    }

    printf("Arquivo gerado em: %s\n", caminho_saida);

    // ALTERAÇÃO: Cálculos de estatísticas corrigidos
    double taxa_acerto_global = (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 100;
    double taxa_acerto_leitura = (stats.total_leituras > 0) ? (double)stats.hits_leitura / stats.total_leituras * 100 : 0;
    double taxa_acerto_escrita = (stats.total_escritas > 0) ? (double)stats.hits_escrita / stats.total_escritas * 100 : 0;
    
    // Tempo médio usando a fórmula: hit_time + miss_rate * miss_penalty
    double miss_rate = 1.0 - ((double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos);
    double tempo_medio = dados_entrada.hit_time + (miss_rate * dados_entrada.tempo_mp_leitura);

    fprintf(arquivo_saida, "SIMULAÇÃO DE CACHE\n\n");
    fprintf(arquivo_saida, "PARÂMETROS DE ENTRADA:\n");
    fprintf(arquivo_saida, "Política de escrita: %s\n", dados_entrada.politica_escrita == 0 ? "Write-Through" : "Write-Back");
    fprintf(arquivo_saida, "Tamanho da linha: %d bytes\n", dados_entrada.tamanho_linha);
    fprintf(arquivo_saida, "Número de linhas: %d\n", dados_entrada.numero_linhas);
    fprintf(arquivo_saida, "Associatividade: %d\n", dados_entrada.associatividade);
    fprintf(arquivo_saida, "Hit time: %d ns\n", dados_entrada.hit_time);
    fprintf(arquivo_saida, "Política de substituição: %s\n", dados_entrada.politica_subs == 0 ? "LRU" : "Aleatória");
    fprintf(arquivo_saida, "Tempo de leitura MP: %d ns\n", dados_entrada.tempo_mp_leitura);
    fprintf(arquivo_saida, "Tempo de escrita MP: %d ns\n\n", dados_entrada.tempo_mp_escrita);

    fprintf(arquivo_saida, "TOTAL DE ENDEREÇOS NO ARQUIVO DE ENTRADA:\n");
    fprintf(arquivo_saida, "Total de endereços: %d\n", stats.total_acessos);
    fprintf(arquivo_saida, "Endereços de leitura: %d\n", stats.total_leituras);
    fprintf(arquivo_saida, "Endereços de escrita: %d\n\n", stats.total_escritas);

    fprintf(arquivo_saida, "TOTAL DE ACESSOS À MEMÓRIA PRINCIPAL:\n");
    fprintf(arquivo_saida, "Leituras da MP: %d\n", stats.leitura_mp);
    fprintf(arquivo_saida, "Escritas na MP: %d\n", stats.escrita_mp);
    fprintf(arquivo_saida, "Total: %d\n\n", stats.leitura_mp + stats.escrita_mp);

    fprintf(arquivo_saida, "TAXA DE ACERTO (HIT RATE):\n");
    fprintf(arquivo_saida, "Taxa de acerto global: %.4f%% (%d hits)\n", taxa_acerto_global, stats.hits_leitura + stats.hits_escrita);
    fprintf(arquivo_saida, "Taxa de acerto leitura: %.4f%% (%d hits)\n", taxa_acerto_leitura, stats.hits_leitura);
    fprintf(arquivo_saida, "Taxa de acerto escrita: %.4f%% (%d hits)\n\n", taxa_acerto_escrita, stats.hits_escrita);

    fprintf(arquivo_saida, "TEMPO MÉDIO DE ACESSO DA CACHE: %.4f ns\n", tempo_medio);

    // Saída no console também
    printf("\nRESULTADOS DA SIMULAÇÃO:\n");
    printf("Taxa de acerto global: %.4f%%\n", taxa_acerto_global);
    printf("Tempo médio de acesso: %.4f ns\n", tempo_medio);
    printf("Total de acessos à MP: %d\n", stats.leitura_mp + stats.escrita_mp);

    fclose(arquivo_saida);
    printf("\nArquivo de saída gerado com sucesso!\n");
}

int main()
{
    FILE *arquivo_entrada;
    char dados_lidos[11], operacao;
    int endereco;

    printf("--------------------------------------------------------------------------------------------------------------------\n");
    printf("|                                                                                                                  |\n");
    printf("|                                            SIMULADOR DE MEMORIA CACHE                                            |\n");
    printf("|                                                                                                                  |\n");
    printf("--------------------------------------------------------------------------------------------------------------------\n");
    printf("|                                                                                                                  |\n");
    printf("|                      Projeto desenvolvido por Andre Colombo Keil e Gabriel Jaskulski Jardim.                     |\n");
    printf("|                                                                                                                  |\n");
    printf("--------------------------------------------------------------------------------------------------------------------\n");

    trata_dados_entrada();

    printf("Caminho Saida: %s\n", caminho_saida);

    if (dados_entrada.arquivo_input == 1)
    {
        arquivo_entrada = fopen("C:\\oficial.cache", "r");
    }
    else
        arquivo_entrada = fopen("C:\\teste.cache", "r");

    if (arquivo_entrada == NULL)
    {
        trata_erro(ARQUIVO_INVALIDO);
    }

    inicializa_cache();
    
    printf("Abriu arquivo com sucesso! Lendo dados...\n");

    // ALTERAÇÃO: Loop principal completamente reescrito
    while (fgets(dados_lidos, 11, arquivo_entrada) != NULL)
    {
        if(sscanf(dados_lidos, "%x %c", &endereco, &operacao) == 2)
        {
            stats.total_acessos++;
            
            if(operacao == 'R')
            {
                stats.total_leituras++;
            }
            else if(operacao == 'W')
            {
                stats.total_escritas++;
            }
            
            // ALTERAÇÃO: Chama nova função para processar o acesso
            processa_acesso_cache(endereco, operacao);
        }
    }
    
    // Se é write-back, escreve linhas dirty na MP
    if(dados_entrada.politica_escrita == 1)
    {
        finaliza_write_back();
    }
  
    fclose(arquivo_entrada);
    cria_arquivo_saida();
    
    // Libera memória
    for(int i = 0; i < informacoes_cache.numero_conjuntos; i++)
    {
        free(conjuntos_cache[i].linhas);
    }
    free(conjuntos_cache);
    
    return 0;
}