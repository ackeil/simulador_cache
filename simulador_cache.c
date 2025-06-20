#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TAMANHO_INT 65535

#define INPUT_INVALIDO 1
#define ARQUIVO_INVALIDO 2

// Função para verificar se um número é potência de 2
int eh_potencia_de_2(int n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

// Função para calcular log2 de um número
int log2_int(int n)
{
    int resultado = 0;
    while (n > 1)
    {
        n >>= 1;
        resultado++;
    }
    return resultado;
}

struct dados_entrada
{
    int politica_escrita;
    int tamanho_linha;
    int numero_linhas;
    int associatividade;
    int hit_time;
    int politica_subs;
    int tempo_mp_leitura;
    int tempo_mp_escrita;
    char caminho_arquivo_inp[65];
    char caminho_arquivo_out[65];
} dados_entrada;

struct estatisticas
{
    int total_acessos;
    int total_leituras;
    int total_escritas;
    int hits_leitura;
    int hits_escrita;
    int misses_leitura;
    int misses_escrita;
    int acessos_mp_leitura;
    int acessos_mp_escrita;
    double tempo_total_acesso;
} stats;

typedef struct linha_cache
{
    int LRU;
    int dirty_bit;
    unsigned int tag;
    int valido;
} str_linha_cache;

typedef struct conjunto_cache
{
    int index_conjunto;
    str_linha_cache **addr_linhas_cache;
} str_conjunto_cache;

typedef struct formato_endereco
{
    int bits_offset; // log2 tamanho_linha
    int bits_indice; // log2 numero_conjuntos
    int bits_tag;    // 32 - bits_offset - bits_indice
} str_formato_endereco;

str_conjunto_cache *cache;
str_formato_endereco formato_end;
int numero_conjuntos;
int contador_lru;

FILE *arquivo_entrada, arquivo_saida;

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

/*
    Printa uma mensagem indicando qual dado de entrada deve ser inputado,
    Uma mensagem indicando o que torna o dado valido
    Joga o dado para a struct de entrada
    Verifica se o dado é valido
*/
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
    } // Tamanho Linha
    printf("Insira o tamanho da linha \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.tamanho_linha);
    if (!eh_potencia_de_2(dados_entrada.tamanho_linha))
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de Linhas
    printf("Insira o numero de linhas \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.numero_linhas);
    if (!eh_potencia_de_2(dados_entrada.numero_linhas))
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de linhas por conjunto
    printf("Insira o numero de linhas por conjunto \n");
    printf("(Deve ser maior que 1 e menor que o total de linhas) \n");
    scanf("%i", &dados_entrada.associatividade);
    if (!eh_potencia_de_2(dados_entrada.associatividade) ||
        dados_entrada.associatividade < 1 ||
        dados_entrada.associatividade > dados_entrada.numero_linhas)
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
    } // Tempo de escrita
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
    printf("Insira o nome do arquivo que contem os dados \n");
    printf("(Somente caracteres ASCII, Maximo de 64 caracteres) \n");
    scanf("%s", dados_entrada.caminho_arquivo_inp);

    printf("Insira o nome do arquivo de saida \n");
    printf("(Somente caracteres ASCII, Maximo de 64 caracteres) \n");
    scanf("%s", dados_entrada.caminho_arquivo_out);
}

// Função para inicializar a cache
void inicializar_cache()
{
    numero_conjuntos = dados_entrada.numero_linhas / dados_entrada.associatividade;

    formato_end.bits_offset = log2_int(dados_entrada.tamanho_linha);
    formato_end.bits_indice = log2_int(numero_conjuntos);
    formato_end.bits_tag = 32 - formato_end.bits_offset - formato_end.bits_indice;

    cache = (str_conjunto_cache *)malloc(numero_conjuntos * sizeof(str_conjunto_cache));

    for (int i = 0; i < numero_conjuntos; i++)
    {
        cache[i].index_conjunto = i;
        cache[i].addr_linhas_cache = (str_linha_cache **)malloc(dados_entrada.associatividade * sizeof(str_linha_cache *));

        for (int j = 0; j < dados_entrada.associatividade; j++)
        {
            cache[i].addr_linhas_cache[j] = (str_linha_cache *)malloc(sizeof(str_linha_cache));
            cache[i].addr_linhas_cache[j]->LRU = 0;
            cache[i].addr_linhas_cache[j]->dirty_bit = 0;
            cache[i].addr_linhas_cache[j]->tag = 0;
            cache[i].addr_linhas_cache[j]->valido = 0;
        }
    }

    // Inicializar estatísticas
    stats.total_acessos = 0;
    stats.total_leituras = 0;
    stats.total_escritas = 0;
    stats.hits_leitura = 0;
    stats.hits_escrita = 0;
    stats.misses_leitura = 0;
    stats.misses_escrita = 0;
    stats.acessos_mp_leitura = 0;
    stats.acessos_mp_escrita = 0;
    stats.tempo_total_acesso = 0.0;
    contador_lru = 0;
}

// Função para extrair partes do endereço
void extrair_endereco(unsigned int endereco, unsigned int *tag, unsigned int *indice, unsigned int *offset)
{
    *offset = endereco & ((1 << formato_end.bits_offset) - 1);
    *indice = (endereco >> formato_end.bits_offset) & ((1 << formato_end.bits_indice) - 1);
    *tag = endereco >> (formato_end.bits_offset + formato_end.bits_indice);
}

// Função para atualizar LRU
void atualizar_lru(int conjunto, int linha)
{
    contador_lru++;
    cache[conjunto].addr_linhas_cache[linha]->LRU = contador_lru;
}

// Função para encontrar linha LRU
int encontrar_lru(int conjunto)
{
    int min_lru = cache[conjunto].addr_linhas_cache[0]->LRU;
    int linha_lru = 0;

    for (int i = 1; i < dados_entrada.associatividade; i++)
    {
        if (cache[conjunto].addr_linhas_cache[i]->LRU < min_lru)
        {
            min_lru = cache[conjunto].addr_linhas_cache[i]->LRU;
            linha_lru = i;
        }
    }
    return linha_lru;
}

// Função para encontrar linha aleatória
int encontrar_aleatorio(int conjunto)
{
    return rand() % dados_entrada.associatividade;
}

// Função principal de simulação da cache
int simular_acesso(unsigned int endereco, char operacao)
{
    unsigned int tag, indice, offset;
    extrair_endereco(endereco, &tag, &indice, &offset);

    // Verificar se é hit
    for (int i = 0; i < dados_entrada.associatividade; i++)
    {
        if (cache[indice].addr_linhas_cache[i]->valido &&
            cache[indice].addr_linhas_cache[i]->tag == tag)
        {
            // HIT
            atualizar_lru(indice, i);

            if (operacao == 'W')
            {
                cache[indice].addr_linhas_cache[i]->dirty_bit = 1;
                stats.hits_escrita++;
                stats.tempo_total_acesso += dados_entrada.hit_time;

                if (dados_entrada.politica_escrita == 0)
                { // Write-through
                    stats.acessos_mp_escrita++;
                    stats.tempo_total_acesso += dados_entrada.tempo_mp_escrita;
                }
            }
            else
            {
                stats.hits_leitura++;
                stats.tempo_total_acesso += dados_entrada.hit_time;
            }
            return 1; // Hit
        }
    }

    // MISS - precisa carregar da memória principal
    stats.acessos_mp_leitura++;
    stats.tempo_total_acesso += dados_entrada.tempo_mp_leitura;

    if (operacao == 'W')
    {
        stats.misses_escrita++;
    }
    else
    {
        stats.misses_leitura++;
    }

    // Encontrar linha para substituição
    int linha_substituir = -1;

    // Primeiro, procurar linha inválida
    for (int i = 0; i < dados_entrada.associatividade; i++)
    {
        if (!cache[indice].addr_linhas_cache[i]->valido)
        {
            linha_substituir = i;
            break;
        }
    }

    // Se não encontrou linha inválida, usar política de substituição
    if (linha_substituir == -1)
    {
        if (dados_entrada.politica_subs == 0)
        { // LRU
            linha_substituir = encontrar_lru(indice);
        }
        else
        { // Aleatória
            linha_substituir = encontrar_aleatorio(indice);
        }

        // Se linha era válida e dirty, escrever na memória principal
        if (cache[indice].addr_linhas_cache[linha_substituir]->valido &&
            cache[indice].addr_linhas_cache[linha_substituir]->dirty_bit &&
            dados_entrada.politica_escrita == 1)
        { // Write-back
            stats.acessos_mp_escrita++;
            stats.tempo_total_acesso += dados_entrada.tempo_mp_escrita;
        }
    }

    // Carregar nova linha
    cache[indice].addr_linhas_cache[linha_substituir]->tag = tag;
    cache[indice].addr_linhas_cache[linha_substituir]->valido = 1;
    cache[indice].addr_linhas_cache[linha_substituir]->dirty_bit = 0;
    atualizar_lru(indice, linha_substituir);

    // Processar operação
    if (operacao == 'W')
    {
        cache[indice].addr_linhas_cache[linha_substituir]->dirty_bit = 1;
        if (dados_entrada.politica_escrita == 0)
        { // Write-through
            stats.acessos_mp_escrita++;
            stats.tempo_total_acesso += dados_entrada.tempo_mp_escrita;
        }
    }

    stats.tempo_total_acesso += dados_entrada.hit_time;
    return 0; // Miss
}

// Função para gerar relatório de saída
void gerar_relatorio()
{
    FILE *arquivo_saida = fopen(dados_entrada.caminho_arquivo_out, "w");
    if (arquivo_saida == NULL)
    {
        printf("Erro ao criar arquivo de saída!\n");
        return;
    }

    fprintf(arquivo_saida, "=== RELATÓRIO DE SIMULAÇÃO DE CACHE ===\n\n");

    // Parâmetros de entrada
    fprintf(arquivo_saida, "PARÂMETROS DE ENTRADA:\n");
    fprintf(arquivo_saida, "Política de escrita: %s\n",
            dados_entrada.politica_escrita == 0 ? "Write-through" : "Write-back");
    fprintf(arquivo_saida, "Tamanho da linha: %d bytes\n", dados_entrada.tamanho_linha);
    fprintf(arquivo_saida, "Número de linhas: %d\n", dados_entrada.numero_linhas);
    fprintf(arquivo_saida, "Associatividade: %d linhas por conjunto\n", dados_entrada.associatividade);
    fprintf(arquivo_saida, "Tempo de hit: %d ns\n", dados_entrada.hit_time);
    fprintf(arquivo_saida, "Política de substituição: %s\n",
            dados_entrada.politica_subs == 0 ? "LRU" : "Aleatória");
    fprintf(arquivo_saida, "Tempo de leitura MP: %d ns\n", dados_entrada.tempo_mp_leitura);
    fprintf(arquivo_saida, "Tempo de escrita MP: %d ns\n", dados_entrada.tempo_mp_escrita);
    fprintf(arquivo_saida, "Arquivo de entrada: %s\n\n", dados_entrada.caminho_arquivo_inp);

    // Estatísticas dos endereços
    fprintf(arquivo_saida, "ESTATÍSTICAS DOS ACESSOS:\n");
    fprintf(arquivo_saida, "Total de acessos: %d\n", stats.total_acessos);
    fprintf(arquivo_saida, "Total de leituras: %d\n", stats.total_leituras);
    fprintf(arquivo_saida, "Total de escritas: %d\n\n", stats.total_escritas);

    // Acessos à memória principal
    fprintf(arquivo_saida, "ACESSOS À MEMÓRIA PRINCIPAL:\n");
    fprintf(arquivo_saida, "Leituras da MP: %d\n", stats.acessos_mp_leitura);
    fprintf(arquivo_saida, "Escritas na MP: %d\n\n", stats.acessos_mp_escrita);

    // Taxas de acerto
    double hit_rate_leitura = stats.total_leituras > 0 ? (double)stats.hits_leitura / stats.total_leituras * 100.0 : 0.0;
    double hit_rate_escrita = stats.total_escritas > 0 ? (double)stats.hits_escrita / stats.total_escritas * 100.0 : 0.0;
    double hit_rate_global = stats.total_acessos > 0 ? (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 100.0 : 0.0;

    fprintf(arquivo_saida, "TAXAS DE ACERTO (HIT RATE):\n");
    fprintf(arquivo_saida, "Hit rate leitura: %.4f%% (%d/%d)\n",
            hit_rate_leitura, stats.hits_leitura, stats.total_leituras);
    fprintf(arquivo_saida, "Hit rate escrita: %.4f%% (%d/%d)\n",
            hit_rate_escrita, stats.hits_escrita, stats.total_escritas);
    fprintf(arquivo_saida, "Hit rate global: %.4f%% (%d/%d)\n\n",
            hit_rate_global, stats.hits_leitura + stats.hits_escrita, stats.total_acessos);

    // Tempo médio de acesso
    double tempo_medio = stats.total_acessos > 0 ? stats.tempo_total_acesso / stats.total_acessos : 0.0;
    fprintf(arquivo_saida, "TEMPO MÉDIO DE ACESSO:\n");
    fprintf(arquivo_saida, "Tempo médio de acesso: %.4f ns\n", tempo_medio);

    fclose(arquivo_saida);
}

// Função para finalizar cache (write-back final)
void finalizar_cache()
{
    if (dados_entrada.politica_escrita == 1)
    { // Write-back
        for (int i = 0; i < numero_conjuntos; i++)
        {
            for (int j = 0; j < dados_entrada.associatividade; j++)
            {
                if (cache[i].addr_linhas_cache[j]->valido &&
                    cache[i].addr_linhas_cache[j]->dirty_bit)
                {
                    stats.acessos_mp_escrita++;
                    stats.tempo_total_acesso += dados_entrada.tempo_mp_escrita;
                }
            }
        }
    }
}

int main()
{
    srand(time(NULL)); // Para política aleatória

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
    inicializar_cache();

    arquivo_entrada = fopen(dados_entrada.caminho_arquivo_inp, "r");

    if (arquivo_entrada == NULL)
    {
        trata_erro(ARQUIVO_INVALIDO);
    }

    printf("Abriu arquivo com sucesso! Processando simulacao...\n");

    char linha[20];
    unsigned int endereco;
    char operacao;

    // Processar cada linha do arquivo
    while (fgets(linha, sizeof(linha), arquivo_entrada) != NULL)
    {
        if (sscanf(linha, "%x %c", &endereco, &operacao) == 2)
        {
            stats.total_acessos++;

            if (operacao == 'R')
            {
                stats.total_leituras++;
            }
            else if (operacao == 'W')
            {
                stats.total_escritas++;
            }

            simular_acesso(endereco, operacao);
        }
    }

    fclose(arquivo_entrada);

    // Finalizar simulação
    finalizar_cache();
    gerar_relatorio();

    printf("Simulacao concluida! Resultados salvos em: %s\n", dados_entrada.caminho_arquivo_out);

    // Liberar memória
    for (int i = 0; i < numero_conjuntos; i++)
    {
        for (int j = 0; j < dados_entrada.associatividade; j++)
        {
            free(cache[i].addr_linhas_cache[j]);
        }
        free(cache[i].addr_linhas_cache);
    }
    free(cache);

    return 0;
}
