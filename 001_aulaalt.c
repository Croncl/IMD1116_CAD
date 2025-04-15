#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define LINHAS 2000
#define COLUNAS 2000
#define TESTES 1000

// Variável global para armazenar o tempo por operação do método mais eficiente
double melhor_tempo_por_op = 0.0;

// Versão eficiente: acesso por linhas
void multiplicarPorLinhas(int matriz[LINHAS][COLUNAS], int vetor[COLUNAS], int resultado[LINHAS]) {
    for (int i = 0; i < LINHAS; i++) {
        resultado[i] = 0;
        for (int j = 0; j < COLUNAS; j++) {
            resultado[i] += matriz[i][j] * vetor[j];
        }
    }
}

// Versão ineficiente: acesso por colunas
void multiplicarPorColunas(int matriz[LINHAS][COLUNAS], int vetor[COLUNAS], int resultado[LINHAS]) {
    for (int j = 0; j < COLUNAS; j++) {
        for (int i = 0; i < LINHAS; i++) {
            resultado[i] += matriz[i][j] * vetor[j];
        }
    }
}

// Função para medir o tempo de execução
void medirTempo(const char* nome, void (*funcao)(int[LINHAS][COLUNAS], int[COLUNAS], int[LINHAS]), 
                int matriz[LINHAS][COLUNAS], int vetor[COLUNAS], int resultado[LINHAS]) {
    struct timespec inicio, fim;
    int64_t tempo_total = 0;
    
    /* 
     * Medição de tempo usando clock_gettime() com CLOCK_MONOTONIC:
     * - CLOCK_MONOTONIC: Mede tempo absoluto, não é afetado por ajustes do sistema
     * - tv_sec: segundos
     * - tv_nsec: nanosegundos (0-999999999)
     */
    for (int t = 0; t < TESTES; t++) {
        // Zerar o vetor resultado antes de cada teste
        for (int i = 0; i < LINHAS; i++) resultado[i] = 0;
        
        clock_gettime(CLOCK_MONOTONIC, &inicio);
        funcao(matriz, vetor, resultado);
        clock_gettime(CLOCK_MONOTONIC, &fim);
        
        // Calcula o tempo decorrido em nanosegundos
        tempo_total += (fim.tv_sec - inicio.tv_sec) * 1000000000LL + (fim.tv_nsec - inicio.tv_nsec);
    }
    
    // Calcula médias
    double tempo_medio = (double)tempo_total / TESTES;
    double tempo_por_op = tempo_medio/(LINHAS*COLUNAS);
    
    // Atualiza o melhor tempo se for o primeiro método ou se for mais rápido
    if (melhor_tempo_por_op == 0.0 || tempo_por_op < melhor_tempo_por_op) {
        melhor_tempo_por_op = tempo_por_op;
    }
    
    // Calcula eficiência relativa ao melhor método
    double eficiencia = (melhor_tempo_por_op / tempo_por_op) * 100;
    
    printf("│ %-16s │ %12.2f ns │ %-8.2f ns/op │ %8.2f%% │\n", 
           nome, tempo_medio, tempo_por_op, eficiencia);
}

void imprimirCabecalho() {
    printf("\n");
    printf("┌──────────────────┬─────────────────┬────────────────┬───────────┐\n");
    printf("│Método            │Tempo Total      │Tempo por Op    │Eficiência │\n");
    printf("├──────────────────┼─────────────────┼────────────────┼───────────┤\n");
}

void imprimirRodape() {
    printf("└──────────────────┴─────────────────┴────────────────┴───────────┘\n");
}

void imprimirParametros() {
    printf("\n════════════════════════════════════════════════════════════════\n");
    printf(" PARÂMETROS DO TESTE\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf(" ▪ Dimensões da matriz: %d x %d\n", LINHAS, COLUNAS);
    printf(" ▪ Número de testes: %d\n", TESTES);
    printf(" ▪ Tipo de dados: int (%zu bytes)\n", sizeof(int));
    printf(" ▪ Tamanho total da matriz: %.2f MB\n", 
           (double)(LINHAS * COLUNAS * sizeof(int)) / (1024 * 1024));
    printf(" ▪ Tamanho do vetor: %.2f KB\n", 
           (double)(COLUNAS * sizeof(int)) / 1024);
    printf("════════════════════════════════════════════════════════════════\n\n");
}

int main() {
    // Alocar matriz e vetor grandes na heap para evitar stack overflow
    static int matriz[LINHAS][COLUNAS];
    static int vetor[COLUNAS];
    static int resultado[LINHAS];
    
    // Inicializar matriz e vetor com valores simples
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLUNAS; j++) {
            matriz[i][j] = i + j;
        }
        vetor[i] = i % 10;
    }
    
    imprimirParametros();
    
    printf(" RESULTADOS DOS TESTES\n");
    printf("════════════════════════════════════════════════════════════════\n");
    
    imprimirCabecalho();
    medirTempo("Por Linhas", multiplicarPorLinhas, matriz, vetor, resultado);
    medirTempo("Por Colunas", multiplicarPorColunas, matriz, vetor, resultado);
    imprimirRodape();
    
    printf("\n LEGENDA:\n");
    printf(" ▪ Tempo Total: Tempo médio para multiplicação completa\n");
    printf(" ▪ Tempo por Op: Tempo médio por operação (matriz[i][j]*vetor[j])\n");
    printf(" ▪ Eficiência: Percentual em relação ao método mais eficiente\n");
    printf("\n OBSERVAÇÕES:\n");
    printf(" - Tempo medido com clock_gettime(CLOCK_MONOTONIC)\n");
    printf(" - %d testes realizados para cada método\n", TESTES);
    printf(" - Eficiência calculada como: (melhor_tempo / tempo_atual) * 100\n");
    
    return 0;
}