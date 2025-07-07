#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// gcc -o simulador_cache.exe simulador_cache.c; echo "0`n128`n16`n4`n5`n1`n70`n70`nC:\resultado_teste.txt" | .\simulador_cache.exe

#define TAMANHO_INT 65535
#define TAMANHO_END 32

#define INPUT_INVALIDO 1
#define ARQUIVO_INVALIDO 2

#define P_E_WB 1
#define P_E_WT 0

#define P_S_ALE 1
#define P_S_LRU 0

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
  int valid;                 
  int tag;                   
  int dirty;                 
  long long int lru_counter; 
} str_linha_cache;

typedef struct conjunto_cache
{
  str_linha_cache *linhas;
} str_conjunto_cache;

typedef struct informacoes_cache
{
  int numero_conjuntos;

  struct
  {
    int rotulo;
    int conjunto;
    int palavra;
  } endereco;

} str_informacoes_cache;

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
  int hit_rate_leituras;
  int hit_rate_escritas;
  int tempo_medio_cache;

  double taxa_acerto_global;
  double taxa_acerto_leitura;
  double taxa_acerto_escrita;
  double hit_rate;
  double miss_rate;
  double tempo_medio;
} stats;

long long int contador_lru;
char caminho_saida[100];

str_dados_entrada dados_entrada;
str_informacoes_cache informacoes_cache;
str_conjunto_cache *conjuntos_cache;

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

int extrai_offset(int endereco)
{
  return endereco & ((1 << informacoes_cache.endereco.palavra) - 1);
}

int extrai_indice_conjunto(int endereco)
{
  return (endereco >> informacoes_cache.endereco.palavra) & ((1 << informacoes_cache.endereco.conjunto) - 1);
}

int extrai_tag(int endereco)
{
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

  printf("Rotulo: %d\n", informacoes_cache.endereco.rotulo);
  printf("Conjunto: %d\n", informacoes_cache.endereco.conjunto);
  printf("Palavra: %d\n", informacoes_cache.endereco.palavra);

  // Aloca a quantidade de conjuntos da cache
  conjuntos_cache = (str_conjunto_cache *)malloc(informacoes_cache.numero_conjuntos * sizeof(str_conjunto_cache));

  for (i = 0; i < informacoes_cache.numero_conjuntos; i++)
  {
    conjuntos_cache[i].linhas = (str_linha_cache *)malloc(dados_entrada.associatividade * sizeof(str_linha_cache));

    // Inicializa todas as linhas como inválidas
    for (j = 0; j < dados_entrada.associatividade; j++)
    {
      conjuntos_cache[i].linhas[j].valid = 0;       // Linha inválida
      conjuntos_cache[i].linhas[j].tag = 0;         // Tag zerado
      conjuntos_cache[i].linhas[j].dirty = 0;       // Não modificado
      conjuntos_cache[i].linhas[j].lru_counter = 0; // Contador LRU
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

// Funcao de busca do LRU
// Recebe o indice do conjunto, e considera o contador LRU da primeira linha
// Passa por todas as linhas, e verifica qual linha possui o menor contador (linha mais antiga)
int busca_linha_lru(int indice_conjunto)
{
  int i, linha_lru = 0;
  long long int menor_contador = conjuntos_cache[indice_conjunto].linhas[0].lru_counter;

  // Busca a linha com menor contador LRU no conjunto específico
  for (i = 1; i < dados_entrada.associatividade; i++)
  {
    if (conjuntos_cache[indice_conjunto].linhas[i].lru_counter < menor_contador)
    {
      menor_contador = conjuntos_cache[indice_conjunto].linhas[i].lru_counter;
      linha_lru = i;
    }
  }

  return linha_lru;
}

// Funcao para simular o acesso a cache
// Recebe qual o endereco desejado e a operacao a ser realizada
// Busca a tag e lida se for hit ou miss
void processa_acesso_cache(int endereco, char operacao)
{
  // Busca a Tag e o indice do conjunto
  int tag = extrai_tag(endereco);
  int indice_conjunto = extrai_indice_conjunto(endereco);
  int i, hit = 0, linha_hit = -1;

  // Busca pela tag no conjunto correto
  for (i = 0; i < dados_entrada.associatividade; i++)
  {
    // Se a linha for valida e a tag for identica, eh hit e salva a linha
    if (conjuntos_cache[indice_conjunto].linhas[i].valid &&
        conjuntos_cache[indice_conjunto].linhas[i].tag == tag)
    {
      hit = 1;
      linha_hit = i;
      break;
    }
  }

  // Se for HIT, atualiuza o contador da LRU, e soma aos stats os valores referentes as operacoes
  if (hit)
  {
    // HIT - atualiza contador LRU
    contador_lru++;
    conjuntos_cache[indice_conjunto].linhas[linha_hit].lru_counter = contador_lru;

    if (operacao == 'R')
    {
      stats.hits_leitura++;
    }
    else if (operacao == 'W')
    {
      stats.hits_escrita++;

      // Marca como dirty se for write-back
      if (dados_entrada.politica_escrita == P_E_WB)
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

    // Busca se ainda ha alguma linha que nao foi populada
    for (i = 0; i < dados_entrada.associatividade; i++)
    {
      if (!conjuntos_cache[indice_conjunto].linhas[i].valid)
      {
        linha_substituir = i;
        break;
      }
    }

    // Se todas linhas estiverem populadas, aplica politica de substituicao
    if (linha_substituir == -1)
    {
      if (dados_entrada.politica_subs == P_S_LRU)
      {
        linha_substituir = busca_linha_lru(indice_conjunto);
      }
      else
      {
        linha_substituir = rand() % dados_entrada.associatividade;
      }

      // Se a linha a ser substituída está dirty, escreve na MP
      if (conjuntos_cache[indice_conjunto].linhas[linha_substituir].dirty)
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
    if (operacao == 'W')
    {
      if (dados_entrada.politica_escrita == P_E_WB) // Write-back
      {
        conjuntos_cache[indice_conjunto].linhas[linha_substituir].dirty = 1;
      }
      else
      {
        stats.escrita_mp++;
      }
    }
  }
}

// Ao finalizar a simulacao, busca se alguma linha ainda ficou com a flag DIRTY e soma statistica de escrita
void finaliza_write_back(void)
{
  int i, j;

  // Passa por todos os conjuntos e todas as linhas
  for (i = 0; i < informacoes_cache.numero_conjuntos; i++)
  {
    for (j = 0; j < dados_entrada.associatividade; j++)
    {
      // Se a linha é válida e está dirty, escreve na MP
      if (conjuntos_cache[i].linhas[j].valid &&
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
  printf("Input: %i\n", dados_entrada.politica_escrita);
  if (dados_entrada.politica_escrita > 1 || dados_entrada.politica_escrita < 0)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Tamanho Linha
  printf("Insira o tamanho da linha \n");
  printf("(Deve ser potencia de 2) \n");
  scanf("%i", &dados_entrada.tamanho_linha);
  printf("Input: %i\n", dados_entrada.tamanho_linha);
  if ((dados_entrada.tamanho_linha & (dados_entrada.tamanho_linha - 1)) != 0)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Numero de Linhas
  printf("Insira o numero de linhas \n");
  printf("(Deve ser potencia de 2) \n");
  scanf("%i", &dados_entrada.numero_linhas);
  printf("Input: %i\n", dados_entrada.numero_linhas);
  if ((dados_entrada.numero_linhas & (dados_entrada.numero_linhas - 1)) != 0)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Numero de linhas por conjunto
  printf("Insira o numero de linhas por conjunto \n");
  printf("(Deve ser maior que 1 e menor que o total de linhas) \n");
  scanf("%i", &dados_entrada.associatividade);
  printf("Input: %i\n", dados_entrada.associatividade);
  if (dados_entrada.associatividade < 1 || dados_entrada.associatividade > dados_entrada.numero_linhas)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Hit Time
  printf("Insira o tempo de acesso da memoria \n");
  printf("(Tempo em Nanossegundos) \n");
  scanf("%i", &dados_entrada.hit_time);
  printf("Input: %i\n", dados_entrada.hit_time);
  if (dados_entrada.hit_time <= 0 || dados_entrada.hit_time > TAMANHO_INT)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Politica de substituicao
  printf("Insira a politica de substituicao \n");
  printf("(0 - LRU, 1 - Aleatoria) \n");
  scanf("%i", &dados_entrada.politica_subs);
  printf("Input: %i\n", dados_entrada.politica_subs);
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
  printf("Input: %i\n", dados_entrada.tempo_mp_leitura);
  if (dados_entrada.tempo_mp_leitura <= 0 || dados_entrada.tempo_mp_leitura > TAMANHO_INT)
  {
    trata_erro(INPUT_INVALIDO);
  }

  // Tempo de escrita
  printf("Insira o tempo de escrita da Memoria Principal \n");
  printf("(Tempo em Nanossegundos) \n");
  scanf("%i", &dados_entrada.tempo_mp_escrita);
  printf("Input: %i\n", dados_entrada.tempo_mp_escrita);
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
  printf("Input: %i\n", dados_entrada.arquivo_input);

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

  stats.taxa_acerto_global = (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos * 10;
  stats.taxa_acerto_leitura = (stats.total_leituras > 0) ? (double)stats.hits_leitura / stats.total_leituras * 10 : 0;
  stats.taxa_acerto_escrita = (stats.total_escritas > 0) ? (double)stats.hits_escrita / stats.total_escritas * 10 : 0;

  // Tempo médio = hit_time + miss_rate * miss_penalty
  // Para write-through, todas as escritas vão para MP, mas isso não afeta o tempo médio da cache

  stats.hit_rate = (double)(stats.hits_leitura + stats.hits_escrita) / stats.total_acessos;
  stats.miss_rate = 1.0 - stats.hit_rate;
  stats.tempo_medio = dados_entrada.hit_time + (stats.miss_rate * dados_entrada.tempo_mp_leitura);

  // Cabeçalho CSV
  fprintf(arquivo_saida, "politica_escrita,tamanho_linha,numero_linhas,associatividade,hit_time,politica_subs,tempo_mp_leitura,tempo_mp_escrita,total_acessos,total_leituras,total_escritas,leitura_mp,escrita_mp,total_mp,taxa_acerto_global,taxa_acerto_leitura,taxa_acerto_escrita,tempo_medio\n");

  // Dados CSV
  fprintf(arquivo_saida, "%s,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%.4f\n",
                dados_entrada.politica_escrita == P_E_WT ? "Write-Through" : "Write-Back",
                dados_entrada.tamanho_linha,
                dados_entrada.numero_linhas,
                dados_entrada.associatividade,
                dados_entrada.hit_time,
                dados_entrada.politica_subs == 0 ? "LRU" : "Aleatoria",
                dados_entrada.tempo_mp_leitura,
                dados_entrada.tempo_mp_escrita,
                stats.total_acessos,
                stats.total_leituras,
                stats.total_escritas,
                stats.leitura_mp,
                stats.escrita_mp,
                stats.leitura_mp + stats.escrita_mp,
                stats.taxa_acerto_global,
                stats.taxa_acerto_leitura,
                stats.taxa_acerto_escrita,
                stats.tempo_medio);

  // Saída no console
  printf("\nRESULTADOS DA SIMULAÇÃO (CSV):\n");
  printf("taxa_acerto_global: %.4f%%\n", stats.taxa_acerto_global);
  printf("tempo_medio: %.4f ns\n", stats.tempo_medio);
  printf("total_acessos_mp: %d\n", stats.leitura_mp + stats.escrita_mp);

  fclose(arquivo_saida);
  printf("\nArquivo de saída gerado com sucesso!\n");
}

int main()
{
  FILE *arquivo_entrada;
  char dados_lidos[11], operacao;
  int endereco, i;

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

  // Busca as linhas do arquivo de entrada
  while (fgets(dados_lidos, 11, arquivo_entrada) != NULL)
  {
    // Pega o endereco e operacao da linha e joga para as variaveis
    if (sscanf(dados_lidos, "%x %c", &endereco, &operacao) == 2)
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

      processa_acesso_cache(endereco, operacao);
    }
  }

  // Se é write-back, escreve linhas dirty na MP
  if (dados_entrada.politica_escrita == 1)
  {
    finaliza_write_back();
  }

  fclose(arquivo_entrada);

  cria_arquivo_saida();

  // Libera memória
  for (i = 0; i < informacoes_cache.numero_conjuntos; i++)
  {
    free(conjuntos_cache[i].linhas);
  }

  free(conjuntos_cache);

  return 0;
}