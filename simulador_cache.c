#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

typedef struct linha_cache
{
    int endereco;
} str_linha_cache;

typedef struct conjunto_cache
{
    int dirty_bit;
    int index_conjunto;
    str_linha_cache **addr_linhas_cache;
} str_conjunto_cache;

// comentarios andre = Coloquei todas as informacoes pra inicializar a cache em uma struct so pra deixar tudo junto
typedef struct informacoes_cache
{
  int numero_conjuntos; // numero_linhas / associatividade
  int mascara_conjuntos;

  struct
  {
    int rotulo;   //  restante dos bits       |   32 - ( conjunto + palavra)
    int conjunto; //  log2 quant conjuntos    |   log2 ( dados_entrada.numero_linhas / dados_entrada.associatividade)
    int palavra;  //  log2 tamanho conjuntos  |   log2 dados_entrada.associatividade
  }endereco;
}str_informacoes_cache;

// comentarios do gabriel = gostei da ideia de usar uma struct para guardar as estatisticas pro isso aderi, caso quiser mudar avisa
// comentarios do andre = alterei a struct pra ter tambem o que o Adami pediu pra estar no arquivo de saida
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

    int quant_enderecos;                  // Total de endereços no arquivo de entrada: especificar o número de endereços de escrita, leitura e a soma dos dois;
    int acessos_mp;                       // Total de escritas e leituras da memória principal;
    int hit_rate;                         // Taxa de acerto (hit rate): especificar esta taxa por leitura, escrita e global (colocar ao lado a quantidade);
    int tempo_medio_cache;                // Tempo médio de acesso da cache (em ns): utilizar a fórmula vista em aula;
} stats;

// Armazena durante o loop qual foi o conjunto menos utilizado
int *array_lru;
int contador_lru;

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

/*
    Calcula o numero de conjuntos
    Formata o endereco baseado nos dados inputados
    Monta a mascara para verificar conjunto
    Inicializa cada conjunto na cache
    Em cada conjunto, inicializa as linhas
*/
void inicializa_cache()
{
    int i, j;
    // Calcula a quantidade de conjuntos
    informacoes_cache.numero_conjuntos    = dados_entrada.numero_linhas / dados_entrada.associatividade;

    // Formata o endereco de acordo com os inputs
    informacoes_cache.endereco.rotulo     = 32 - (informacoes_cache.endereco.palavra + informacoes_cache.endereco.conjunto);
    informacoes_cache.endereco.conjunto   = log2(informacoes_cache.numero_conjuntos);
    informacoes_cache.endereco.palavra    = log2(dados_entrada.tamanho_linha);

    // Deixa pronta a mascara para os conjuntos
    informacoes_cache.mascara_conjuntos   = (int)(pow(2, informacoes_cache.endereco.conjunto) - 1) << informacoes_cache.endereco.palavra;

    // Aloca a quantidade de conjuntos da cache
    conjuntos_cache = (str_conjunto_cache*) malloc (informacoes_cache.numero_conjuntos * (sizeof(str_conjunto_cache)));

    // Passa por cada conjunto e aloca as linhas de cada um deles
    for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
    {
      conjuntos_cache[i].index_conjunto    = 0;
      conjuntos_cache[i].dirty_bit         = 0;
      conjuntos_cache[i].addr_linhas_cache = (str_linha_cache **) malloc (dados_entrada.associatividade * sizeof(str_linha_cache*));

      for(j = 0; j < dados_entrada.associatividade; j++)
      {
        conjuntos_cache[i].addr_linhas_cache[j] = (str_linha_cache *) malloc (dados_entrada.associatividade * sizeof(str_linha_cache));

        conjuntos_cache[i].addr_linhas_cache[j]->endereco = 0;
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
    stats.quant_enderecos = 0;
    stats.acessos_mp = 0;
    stats.hit_rate = 0;
    stats.tempo_medio_cache = 0;
    contador_lru = 0;
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
    if (dados_entrada.tamanho_linha % 2 != 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de Linhas
    printf("Insira o numero de linhas \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.numero_linhas);
    if (dados_entrada.numero_linhas % 2 != 0)
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
    scanf("%i", &dados_entrada.tempo_mp_leitura);
    if (dados_entrada.tempo_mp_leitura <= 0 || dados_entrada.tempo_mp_leitura > TAMANHO_INT)
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

// comentarios do andre - Alterei o nome da funcao pra ficar mais proximo do que ela realmente faz
void cria_arquivo_saida()
{
    FILE *arquivo_saida = fopen(caminho_saida, "w+");
    if (arquivo_saida == NULL)
    {
        printf("Erro ao criar arquivo de saída!\n");
        return;
    }
    fprintf(arquivo_saida, "SIMULAÇÃO DE CACHE\n\n");

    // comentarios do gabriel = colocar os parametros de entrada???
    // comentarios do andre = Colocado meu patrao

    fprintf(arquivo_saida, "DADOS ENTRADA:\n");

    fprintf(arquivo_saida, "Politica de escrita: ");
    if(dados_entrada.politica_escrita == 0)
    {
      fprintf(arquivo_saida, "Write-Through\n");
    }
    else fprintf(arquivo_saida, "Write-Back\n");

    fprintf(arquivo_saida, "Tamanho da linha: %d\n", dados_entrada.tamanho_linha);
    fprintf(arquivo_saida, "Numero de Linhas: %d\n", dados_entrada.numero_linhas);
    fprintf(arquivo_saida, "Associatividade (Linhas por Conjunto): %d\n", dados_entrada.associatividade);
    fprintf(arquivo_saida, "Hit Time: %d\n", dados_entrada.hit_time);

    fprintf(arquivo_saida, "Politica de Substituicao: ");
    if(dados_entrada.politica_subs == 0)
    {
      fprintf(arquivo_saida, "LRU\n");
    }
    else fprintf(arquivo_saida, "Aleatorio\n");

    fprintf(arquivo_saida, "Tempo de Leitura MP: %d\n", dados_entrada.tempo_mp_leitura);
    fprintf(arquivo_saida, "Tempo de Escrita MP: %d\n", dados_entrada.tempo_mp_escrita);

    fprintf(arquivo_saida, "Arquivo de Entrada: ");
    if(dados_entrada.arquivo_input == 0)
    {
      fprintf(arquivo_saida, "teste.cache\n");
    }
    else fprintf(arquivo_saida, "oficial.cache\n");

    fprintf(arquivo_saida, "ACESSOS:\n");
    fprintf(arquivo_saida, "Total de acessos: %d\n", stats.total_acessos);
    fprintf(arquivo_saida, "Total de leituras: %d\n", stats.total_leituras);
    fprintf(arquivo_saida, "Total de escritas: %d\n\n", stats.total_escritas);

    fprintf(arquivo_saida, "HITS:\n");
    fprintf(arquivo_saida, "Hits de leitura: %d\n", stats.hits_leitura);
    fprintf(arquivo_saida, "Hits de escrita: %d\n", stats.hits_escrita);
    fprintf(arquivo_saida, "Misses de leitura: %d\n", stats.misses_leitura);
    fprintf(arquivo_saida, "Misses de escrita: %d\n\n", stats.misses_escrita);

    fprintf(arquivo_saida, "MEMÓRIA PRINCIPAL:\n");
    fprintf(arquivo_saida, "Acessos à memória principal (leitura): %d\n", stats.acessos_mp_leitura);
    fprintf(arquivo_saida, "Acessos à memória principal (escrita): %d\n", stats.acessos_mp_escrita);
    fprintf(arquivo_saida, "Tempo total de acesso: %.4f ns\n", stats.tempo_total_acesso);
    fprintf(arquivo_saida, "Tempo médio de acesso da cache: %.4f ns\n", stats.tempo_total_acesso / stats.total_acessos);
    fprintf(arquivo_saida, "Taxa de acerto (hit rate): %.4f%%\n", (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 100);

    // comentarios do gabriel = fazer calculo de porcentagem de acertos e tempo medio???

    fclose(arquivo_saida);
    printf("Arquivo de saída gerado com sucesso!\n");

    // comentarios do gabriel = ta agardavel assim?
}

int main()
{
    FILE *arquivo_entrada;
    char dados_lidos[11];
    int i = 0;

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

    //  comentarios do gabriel = Pedir caminho para os arquivos de entrada?? pq fiquei com preguica de mudar o arquivo pra esse caminho ai
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

    // comentarios do andre - Joguei a inicializacao da cache pra depois da verificacao do arquivo de input
    inicializa_cache();
    
    printf("Abriu arquivo com sucesso! Lendo dados...\n");

    // Le todos os dados do arquivo de entrada e printa
    while (fgets(dados_lidos, 10, arquivo_entrada) != NULL)
    {
      // comentarios do andre = prototipo de uma logica que acho que podemos usar
      /*
          // Passa por todos os conjuntos
          for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
          {
            if(endereco_lido & conjuntos_cache[i].mascara_conjunto)
            {
              quant_hits++;
              tempo_memoria += informacoes_cache.hit_time;
            }
            else
            {
              // Implementar logica do LRU e substituir conjunto na Cache
              busca_conjunto_mp();
              quant_miss++;
              tempo_memoria += informacoes_cache.tempo_mp;
            }
          }
      
      */

        printf("%i: %s\n", i++, dados_lidos);
    }
    /*
    versao do copiloto
    comentarios do gabriel = achei genial o sizeof(linha) o resto eh bem normal, creio que podemos usar desse jeito
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

                simular_acesso(endereco, operacao);  comentarios do gabriel = essa eh a funcao principal que vai fazer a simulacao, nao copiei a do coppiloto pra nos fazer !!!
            }
        }

        fclose(arquivo_entrada);


   comentarios do gabriel = copiloto disse para fazer essa funcao depois de passar na cache para fazer o write-back final, preocede??
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
    */
    cria_arquivo_saida();
    return 0;

    // Ler os dados do arquivo e alocar a memoria para usar

    /*
        Estrutura do Loop
            Verifica dado de entrada
            Busca Conjunto do endereço
            Verifica se conjunto na memoria Cache
            Sim?
                Soma tempo de leitura da memoria cache
                Atualiza LRU
                Sai
            Nao?
                Verifica se cache ainda com espaço
                Tira LRU
                Coloca conjunto requisitado no lugar
                Soma tempo de leitura M.P
                Sai
    */
    //

    // No fim do loop gera arquivo de saida
    /*
        Todos os parâmetros de entrada: assim é possível verificar os parâmetros utilizados;
        Total de endereços no arquivo de entrada: especificar o número de endereços de escrita, leitura e a soma dos dois;
        Total de escritas e leituras da memória principal;
        Taxa de acerto (hit rate): especificar esta taxa por leitura, escrita e global (colocar ao lado a quantidade);
        Tempo médio de acesso da cache (em ns): utilizar a fórmula vista em aula;
        Todas as saídas que forem números reais devem ter 4 casas decimais. Ao término da simulação, deve-se atualizar a memória principal com as caches alteradas (caso necessário).
    */
}
