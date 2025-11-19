#include <stdio.h>
#include <stdlib.h>
#include "gera_codigo.h"

int main(int argc, char *argv[]) {
    FILE *fp;
    funcp funcLBS = NULL;
    unsigned char code[1024];
    int res;
    int argumento = 0; // Valor padrão caso você não digite nada

    /* 1. Verifica argumentos básicos */
    if (argc < 2) {
        printf("Uso: %s <arquivo_lbs> [numero_entrada]\n", argv[0]);
        return 1;
    }

    /* 2. Abre o arquivo */
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Erro ao abrir o arquivo: %s\n", argv[1]);
        return 1;
    }

    /* 3. Gera o codigo */
    gera_codigo(fp, code, &funcLBS);
    fclose(fp); 

    if (funcLBS == NULL) {
        printf("Erro na geracao do codigo.\n");
        return 1;
    }

    /* 4. Pega o número digitado no terminal (Se existir) */
    if (argc > 2) {
        argumento = atoi(argv[2]); // Converte texto para inteiro (ASCII to Int)
    } else {
        printf("-> Nenhum numero fornecido. Usando 0 como padrao.\n");
    }

    /* 5. Executa */
    printf("Executando função com entrada: %d\n", argumento);
    res = (*funcLBS)(argumento); 
    printf("Resultado retornado: %d\n", res);

    return 0;
}