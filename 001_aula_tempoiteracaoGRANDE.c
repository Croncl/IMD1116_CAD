#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define LINHAS 8000
#define COLUNAS 8000

// Variáveis para armazenar os tempos da primeira operação por linha ou coluna
uint64_t tempos_primeira_linha[LINHAS];
uint64_t tempos_primeira_coluna[COLUNAS];

// Matrizes e vetores alocados estaticamente
int matriz[LINHAS][COLUNAS];
int vetor[COLUNAS];
int resultado[LINHAS];

// Função para multiplicação por linhas (medindo apenas o tempo da primeira operação de cada linha)
void multiplicarPorLinhas() {
    struct timespec inicio, fim;

    for (int i = 0; i < LINHAS; i++) {
        resultado[i] = 0;

        // Medir apenas o tempo da primeira operação da linha
        clock_gettime(CLOCK_MONOTONIC, &inicio);
        resultado[i] += matriz[i][0] * vetor[0];
        clock_gettime(CLOCK_MONOTONIC, &fim);

        tempos_primeira_linha[i] = (fim.tv_sec - inicio.tv_sec) * 1000000000LL + 
                                   (fim.tv_nsec - inicio.tv_nsec);

        // Continuar com as demais operações sem medir o tempo
        for (int j = 1; j < COLUNAS; j++) {
            resultado[i] += matriz[i][j] * vetor[j];
        }
    }
}

// Função para multiplicação por colunas (medindo apenas o tempo da primeira operação de cada coluna)
void multiplicarPorColunas() {
    struct timespec inicio, fim;

    for (int j = 0; j < COLUNAS; j++) {
        for (int i = 0; i < LINHAS; i++) {
            if (i == 0) {
                // Medir apenas o tempo da primeira operação da coluna
                clock_gettime(CLOCK_MONOTONIC, &inicio);
                resultado[i] += matriz[i][j] * vetor[j];
                clock_gettime(CLOCK_MONOTONIC, &fim);

                tempos_primeira_coluna[j] = (fim.tv_sec - inicio.tv_sec) * 1000000000LL + 
                                            (fim.tv_nsec - inicio.tv_nsec);
            } else {
                // Continuar com as demais operações sem medir o tempo
                resultado[i] += matriz[i][j] * vetor[j];
            }
        }
    }
}

// Função para imprimir os tempos da primeira operação em formato de tabela com 4 colunas
void imprimirTemposTabela(const char* titulo, uint64_t tempos[], int tamanho) {
    printf("\n%s:\n", titulo);
    for (int i = 0; i < tamanho; i++) {
        printf("Índice %4d: %6lu ns\t", i, tempos[i]);
        if ((i + 1) % 4 == 0) { // Quebra de linha a cada 4 colunas
            printf("\n");
        }
    }
    if (tamanho % 4 != 0) {
        printf("\n"); // Garantir quebra de linha no final
    }
}

int main() {
    // Inicializar a matriz e o vetor com valores
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLUNAS; j++) {
            matriz[i][j] = i + j + 1;
        }
    }
    for (int j = 0; j < COLUNAS; j++) {
        vetor[j] = j + 1;
    }

    printf("----------------------------------------\n");
    printf("  MEDIÇÃO DA PRIMEIRA OPERAÇÃO POR LINHA\n");
    printf("----------------------------------------\n");

    multiplicarPorLinhas();
    imprimirTemposTabela("Tempos da primeira operação por LINHA (ns)", tempos_primeira_linha, LINHAS);

    printf("----------------------------------------\n");
    printf("  MEDIÇÃO DA PRIMEIRA OPERAÇÃO POR COLUNA\n");
    printf("----------------------------------------\n");

    // Zerar o resultado antes de medir por colunas
    for (int i = 0; i < LINHAS; i++) resultado[i] = 0;
    multiplicarPorColunas();
    imprimirTemposTabela("Tempos da primeira operação por COLUNA (ns)", tempos_primeira_coluna, COLUNAS);

    return 0;
}