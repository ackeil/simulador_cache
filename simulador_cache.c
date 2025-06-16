#include <stdio.h>
#include <stdlib.h>

#define TAMANHO_INT         65535

#define INPUT_INVALIDO      1
#define ARQUIVO_INVALIDO    2

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
    char caminho_arquivo_inp[129];
}dados_entrada;

typedef struct linha_cache
{
    int LRU;
    int dirty_bit;
    int endereco;
}str_linha_cache;

FILE* arquivo_entrada, arquivo_saida;

void trata_erro(int erro)
{
    printf("ERRO!");

    switch(erro)
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
    if(dados_entrada.politica_escrita > 1 || dados_entrada.politica_escrita < 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Tamanho Linha
    printf("Insira o tamanho da linha \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.tamanho_linha);
    if(dados_entrada.tamanho_linha % 2 != 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de Linhas
    printf("Insira o numero de linhas \n");
    printf("(Deve ser potencia de 2) \n");
    scanf("%i", &dados_entrada.numero_linhas);
    if(dados_entrada.numero_linhas % 2 != 0)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Numero de linhas por conjunto
    printf("Insira o numero de linhas por conjunto \n");
    printf("(Deve ser maior que 1 e menor que o total de linhas) \n");
    scanf("%i", &dados_entrada.associatividade);
    if(dados_entrada.associatividade < 1 || dados_entrada.associatividade > dados_entrada.numero_linhas)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Hit Time
    printf("Insira o tempo de acesso da memoria \n");
    printf("(Tempo em Nanossegundos) \n");
    scanf("%i", &dados_entrada.hit_time);
    if(dados_entrada.hit_time <= 0 || dados_entrada.hit_time > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Politica de substituicao
    printf("Insira a politica de substituicao \n");
    printf("(0 - LRU, 1 - Aleatoria) \n");
    scanf("%i", &dados_entrada.politica_subs);
    if(dados_entrada.politica_subs > 1 || dados_entrada.politica_subs < 0)
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
    if(dados_entrada.tempo_mp_leitura <= 0 || dados_entrada.tempo_mp_leitura > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    // Tempo de escrita
    printf("Insira o tempo de escrita da Memoria Principal \n");
    printf("(Tempo em Nanossegundos) \n");
    scanf("%i", &dados_entrada.tempo_mp_leitura);
    if(dados_entrada.tempo_mp_leitura <= 0 || dados_entrada.tempo_mp_leitura > TAMANHO_INT)
    {
        trata_erro(INPUT_INVALIDO);
    }

    printf("-------------------------------------------------------------------------\n");
    printf("|                                                                       |\n");
    printf("|                         Arquivo dos Dados                             |\n");
    printf("|                                                                       |\n");
    printf("-------------------------------------------------------------------------\n");

    printf("Insira o nome do arquvio que contem os dados \n");
    printf("(Somente caracteres ASCII, Maximo de 128 caracteres) \n");
    scanf("%s", dados_entrada.caminho_arquivo_inp);
}

int main()
{                                                                                                

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

    arquivo_entrada = fopen(dados_entrada.caminho_arquivo_inp,"r");

    printf("Caminho do arquivo: ");
    printf(dados_entrada.caminho_arquivo_inp);

    if(arquivo_entrada == NULL)
    {
        trata_erro(ARQUIVO_INVALIDO);
    }

    return 0;
}
