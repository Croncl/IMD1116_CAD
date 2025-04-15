#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define LINHAS 6
#define COLUNAS 6

// Variáveis para armazenar os tempos de cada operação
uint64_t tempos_por_linha[LINHAS][COLUNAS];
uint64_t tempos_por_coluna[LINHAS][COLUNAS];

// Função para multiplicação por linhas (com medição de cada acesso)
void multiplicarPorLinhas(int matriz[LINHAS][COLUNAS], int vetor[COLUNAS], int resultado[LINHAS]) {
    struct timespec inicio, fim;

    for (int i = 0; i < LINHAS; i++) {
        resultado[i] = 0;
        for (int j = 0; j < COLUNAS; j++) {
            // Mede apenas o tempo do acesso + multiplicação
            clock_gettime(CLOCK_MONOTONIC, &inicio);
            resultado[i] += matriz[i][j] * vetor[j];
            clock_gettime(CLOCK_MONOTONIC, &fim);

            tempos_por_linha[i][j] = (fim.tv_sec - inicio.tv_sec) * 1000000000LL + 
                                    (fim.tv_nsec - inicio.tv_nsec);
        }
    }
}

// Função para multiplicação por colunas (com medição de cada acesso)
void multiplicarPorColunas(int matriz[LINHAS][COLUNAS], int vetor[COLUNAS], int resultado[LINHAS]) {
    struct timespec inicio, fim;

    for (int j = 0; j < COLUNAS; j++) {
        for (int i = 0; i < LINHAS; i++) {
            // Mede apenas o tempo do acesso + multiplicação
            clock_gettime(CLOCK_MONOTONIC, &inicio);
            resultado[i] += matriz[i][j] * vetor[j];
            clock_gettime(CLOCK_MONOTONIC, &fim);

            tempos_por_coluna[i][j] = (fim.tv_sec - inicio.tv_sec) * 1000000000LL + 
                                     (fim.tv_nsec - inicio.tv_nsec);
        }
    }
}

// Função para imprimir os tempos de forma organizada
void imprimirTempos(const char* titulo, uint64_t tempos[LINHAS][COLUNAS]) {
    printf("\n%s:\n", titulo);
    printf("      ");
    for (int j = 0; j < COLUNAS; j++) {
        printf(" Col%-2d ", j);
    }
    printf("\n");

    for (int i = 0; i < LINHAS; i++) {
        printf("Lin%-2d: ", i);
        for (int j = 0; j < COLUNAS; j++) {
            printf("%6lu ns ", tempos[i][j]);
        }
        printf("\n");
    }
}

int main() {
    int matriz[LINHAS][COLUNAS] = {
        {1, 2, 3, 4, 5, 6},
        {7, 8, 9, 10, 11, 12},
        {13, 14, 15, 16, 17, 18},
        {19, 20, 21, 22, 23, 24},
        {25, 26, 27, 28, 29, 30},
        {31, 32, 33, 34, 35, 36}
    };

    int vetor[COLUNAS] = {1, 2, 3, 4, 5, 6};
    int resultado[LINHAS] = {0};

    printf("----------------------------------------\n");
    printf("  MEDIÇÃO INDIVIDUAL DE TEMPOS DE ACESSO\n");
    printf("----------------------------------------\n");

    // Zerar o resultado antes de cada teste
    for (int i = 0; i < LINHAS; i++) resultado[i] = 0;
    multiplicarPorLinhas(matriz, vetor, resultado);

    for (int i = 0; i < LINHAS; i++) resultado[i] = 0;
    multiplicarPorColunas(matriz, vetor, resultado);

    // Imprimir os tempos
    imprimirTempos("Tempos por LINHA (ns)", tempos_por_linha);
    imprimirTempos("Tempos por COLUNA (ns)", tempos_por_coluna);

    return 0;
}