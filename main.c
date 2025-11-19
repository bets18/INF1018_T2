#include <stdio.h>
#include <stdlib.h>
#include "gera_codigo.h"

int main(int argc, char *argv[]) {
    FILE *fp;
    funcp funcLBS = NULL; // Ponteiro para a função gerada
    unsigned char code[1024]; // Vetor onde o código de máquina será gravado
    int res;

    /* Verifica se o usuário passou o nome do arquivo de entrada */
    if (argc < 2) {
        printf("Uso: %s <arquivo_lbs>\n", argv[0]);
        return 1;
    }

    /* Abre o arquivo para leitura */
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Erro ao abrir o arquivo: %s\n", argv[1]);
        return 1;
    }

    /* Gera o codigo */
    // Passamos o arquivo, o vetor de buffer e o endereço do ponteiro da função
    gera_codigo(fp, code, &funcLBS);
    
    // Fecha o arquivo pois não precisamos mais dele
    fclose(fp); 

    /* Verifica se algo foi gerado */
    if (funcLBS == NULL) {
        printf("Erro na geracao do codigo ou arquivo vazio.\n");
        return 1;
    }

    /* Chama a função gerada */
    // Estou passando '10' como argumento de teste padrão.
    // Se a sua função LBS usar 'p0', ela receberá 10.
    printf("Iniciando execucao da funcao gerada...\n");
    res = (*funcLBS)(10); 

    printf("Resultado retornado: %d\n", res);

    return 0;
}