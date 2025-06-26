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

typedef struct linha_cache
{
    int endereco;
} str_linha_cache;

typedef struct conjunto_cache
{
    int dirty_bit;
    int index_conjunto;
    long long int lru;
    str_linha_cache **addr_linhas_cache;
} str_conjunto_cache;

// comentarios andre = Coloquei todas as informacoes pra inicializar a cache em uma struct so pra deixar tudo junto
typedef struct informacoes_cache
{
  int numero_conjuntos; // numero_linhas / associatividade
  int mascara_conjuntos;
  int index_ocupacao_cache;

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
    int leitura_mp;
    int escrita_mp;

    double tempo_total_acesso;

    int quant_enderecos;                  // Total de endereços no arquivo de entrada: especificar o número de endereços de escrita, leitura e a soma dos dois;
    int acessos_mp;                       // Total de escritas e leituras da memória principal;
    int hit_rate;                         // Taxa de acerto (hit rate): especificar esta taxa por leitura, escrita e global (colocar ao lado a quantidade);
    int hit_rate_leituras;
    int hit_rate_escritas;
    int tempo_medio_cache;                // Tempo médio de acesso da cache (em ns): utilizar a fórmula vista em aula;
} stats;

// Armazena durante o loop qual foi o conjunto menos utilizado
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
    informacoes_cache.endereco.palavra    = log2(dados_entrada.tamanho_linha);
    informacoes_cache.endereco.conjunto   = log2(informacoes_cache.numero_conjuntos);
    informacoes_cache.endereco.rotulo     = TAMANHO_END - (informacoes_cache.endereco.palavra + informacoes_cache.endereco.conjunto);

    // Deixa pronta a mascara para os conjuntos
    informacoes_cache.mascara_conjuntos   = (int)(pow(2, informacoes_cache.endereco.conjunto) - 1) << informacoes_cache.endereco.palavra;

    printf("Rotulo: %d\n", informacoes_cache.endereco.rotulo);
    printf("Conjunto: %d\n", informacoes_cache.endereco.conjunto);
    printf("Palavra: %d\n", informacoes_cache.endereco.palavra);
    printf("Mascara: %x\n", informacoes_cache.mascara_conjuntos);

    // Aloca a quantidade de conjuntos da cache
    conjuntos_cache = (str_conjunto_cache*) malloc (informacoes_cache.numero_conjuntos * (sizeof(str_conjunto_cache)));

    // Passa por cada conjunto e aloca as linhas de cada um deles
    for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
    {
      conjuntos_cache[i].index_conjunto    = 0;
      conjuntos_cache[i].dirty_bit         = 0;
      conjuntos_cache[i].addr_linhas_cache = (str_linha_cache **) malloc (dados_entrada.associatividade * sizeof(str_linha_cache*));

      // comentario do andre = A gente precisa saber qualquer coisa sobre as linhas? teoricamente vai estar tudo no conjunto, acho que facilita na implementacao
      for(j = 0; j < dados_entrada.associatividade; j++)
      {
        conjuntos_cache[i].addr_linhas_cache[j] = (str_linha_cache *) malloc (dados_entrada.associatividade * sizeof(str_linha_cache));

        conjuntos_cache[i].addr_linhas_cache[j]->endereco = 0;
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

int busca_lru()
{
  int i, aux;

  aux = conjuntos_cache[0].lru;

  // Passa por todos os conjuntos
  for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
  {
    // ISSO ta BEM errado
    if(aux > conjuntos_cache[i].lru)aux = i;
  }

  return aux;
}

void busca_conjunto_mp(int endereco)
{
  int conjunto_atualizado;

  printf("Buscando conjunto na memoria principal!\n");

  printf("Index Ocupacao Cache: %d\n", informacoes_cache.index_ocupacao_cache);

  // Se a cache esta cheia, busca qual conjunto sera ocupado
  if(informacoes_cache.index_ocupacao_cache > informacoes_cache.numero_conjuntos)
  {
    // Busca o conjunto que vai ser esvaziado de forma aleatoria
    if(dados_entrada.politica_subs == 1)
    {
      conjunto_atualizado = rand() % informacoes_cache.numero_conjuntos;
    }
    if(dados_entrada.politica_subs == 0)
    {
      conjunto_atualizado = busca_lru();
    }
  }
  else conjunto_atualizado = informacoes_cache.index_ocupacao_cache++;

  conjuntos_cache[conjunto_atualizado].index_conjunto = (endereco & informacoes_cache.mascara_conjuntos);

  printf("Conjunto Atualizado: %i\n", conjunto_atualizado);
  printf("Index Conjunto: %x\n", conjuntos_cache[conjunto_atualizado].index_conjunto);
}

void finaliza_write_back(void)
{
  int i;

  // Passa por todos os conjuntos
  for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
  {
    if(conjuntos_cache[i].dirty_bit == 1)
    {
      stats.total_escritas++;
      stats.escrita_mp++;
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

    fprintf(arquivo_saida, "SIMULAÇÃO DE CACHE\n\n");

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
    fprintf(arquivo_saida, "Misses de leitura: %d\n", stats.leitura_mp);
    fprintf(arquivo_saida, "Misses de escrita: %d\n\n", stats.escrita_mp);

    stats.tempo_total_acesso =  (stats.hits_leitura * dados_entrada.hit_time) + 
                                (stats.hits_escrita * dados_entrada.hit_time) +
                                (stats.leitura_mp * dados_entrada.tempo_mp_leitura) + 
                                (stats.escrita_mp * dados_entrada.tempo_mp_escrita);

    fprintf(arquivo_saida, "MEMÓRIA PRINCIPAL:\n");
    fprintf(arquivo_saida, "Tempo total de acesso: %.4f ns\n", stats.tempo_total_acesso);
    fprintf(arquivo_saida, "Tempo médio de acesso da cache: %.4f ns\n", stats.tempo_total_acesso / stats.total_acessos);
    fprintf(arquivo_saida, "Taxa de acerto (hit rate): %.4f%%\n", (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 100);

    // comentarios do gabriel = fazer calculo de porcentagem de acertos e tempo medio???

    printf("SIMULAÇÃO DE CACHE\n\n");

    printf("DADOS ENTRADA:\n");

    printf("Politica de escrita: ");
    if(dados_entrada.politica_escrita == 0)
    {
      printf("Write-Through\n");
    }
    else printf("Write-Back\n");

    printf("Tamanho da linha: %d\n", dados_entrada.tamanho_linha);
    printf("Numero de Linhas: %d\n", dados_entrada.numero_linhas);
    printf("Associatividade (Linhas por Conjunto): %d\n", dados_entrada.associatividade);
    printf("Hit Time: %d\n", dados_entrada.hit_time);

    printf("Politica de Substituicao: ");
    if(dados_entrada.politica_subs == 0)
    {
      printf("LRU\n");
    }
    else printf("Aleatorio\n");

    printf("Tempo de Leitura MP: %d\n", dados_entrada.tempo_mp_leitura);
    printf("Tempo de Escrita MP: %d\n", dados_entrada.tempo_mp_escrita);

    printf("Arquivo de Entrada: ");
    if(dados_entrada.arquivo_input == 0)
    {
      printf("teste.cache\n");
    }
    else printf("oficial.cache\n");

    printf("ACESSOS:\n");
    printf("Total de acessos: %d\n", stats.total_acessos);
    printf("Total de leituras: %d\n", stats.total_leituras);
    printf("Total de escritas: %d\n\n", stats.total_escritas);

    printf("HITS:\n");
    printf("Hits de leitura: %d\n", stats.hits_leitura);
    printf("Hits de escrita: %d\n", stats.hits_escrita);
    printf("Misses de leitura: %d\n", stats.leitura_mp);
    printf("Misses de escrita: %d\n\n", stats.escrita_mp);

    stats.tempo_total_acesso =  (stats.hits_leitura * dados_entrada.hit_time) + 
                                (stats.hits_escrita * dados_entrada.hit_time) +
                                (stats.leitura_mp * dados_entrada.tempo_mp_leitura) + 
                                (stats.escrita_mp * dados_entrada.tempo_mp_escrita);

    printf("MEMÓRIA PRINCIPAL:\n");
    printf("Tempo total de acesso: %.4f ns\n", stats.tempo_total_acesso);
    printf("Tempo médio de acesso da cache: %.4f ns\n", stats.tempo_total_acesso / stats.total_acessos);
    printf("Taxa de acerto (hit rate): %.4f%%\n", (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 100);

    fclose(arquivo_saida);

    printf("Arquivo de saida gerado com sucesso!\n");
}

int main()
{
    FILE *arquivo_entrada;
    char dados_lidos[11], operacao;
    int endereco;
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

    // Le todos os dados do arquivo de entrada e printa
    while (fgets(dados_lidos, 11, arquivo_entrada) != NULL)
    {
      if(sscanf(dados_lidos, "%x %c", &endereco, &operacao) == 2)
      {
        // printf("Endereco: %x\nOperacao: %c\n", endereco, operacao);

        stats.total_acessos++;
        
        // Passa por todos os conjuntos
        for(i = 0; i < informacoes_cache.numero_conjuntos; i++)
        {
          // printf("I: %i\nIndex Conjunto: %x\n", i, conjuntos_cache[i].index_conjunto);
        
          if(
              ((endereco & conjuntos_cache[i].index_conjunto) == conjuntos_cache[i].index_conjunto) &&
              ((endereco & conjuntos_cache[i].index_conjunto) != 0)
            )
          {
            contador_lru++;
    
            conjuntos_cache[i].lru = contador_lru;

            if(operacao == 'W')
            {
              stats.total_escritas++;
              stats.hits_escrita++;
              // Adcionar logica para write-through
              // Precisa de logica para WT??
            }
            if(operacao == 'R')
            {
              stats.total_leituras++;
              stats.hits_leitura++;
            }

            break;
          }
        }

        // Se chegou no ultimo elemento e nao deu hit
        if(i == informacoes_cache.numero_conjuntos)
        {
          if(operacao == 'W')
          {
            stats.total_escritas++;
            stats.escrita_mp++;
          }
          if(operacao == 'R')
          {
            stats.total_leituras++;
            stats.leitura_mp++;
          }
          
          busca_conjunto_mp(endereco);
        }
      }
    }
  
    fclose(arquivo_entrada);
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
        Todas as saidas que forem números reais devem ter 4 casas decimais. Ao término da simulação, deve-se atualizar a memória principal com as caches alteradas (caso necessário).
    */
}
