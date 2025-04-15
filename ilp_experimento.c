#include <stdio.h>
#include <sys/time.h>

#define SIZE 100000000

// Alocação estática dos vetores
int a[SIZE];
int b[SIZE];

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    double start, end;
    int sum;
    
    // 1. Inicialização com cálculo simples
    start = get_time();
    for (int i = 0; i < SIZE; i++) {
        a[i] = i+1;
    }
    end = get_time();
    printf("----------------------TESTE----------------------\n");
    printf("Tempo inicializacao: %.6f segundos\n", end - start);
    
    // 2. Soma cumulativa com dependência
    sum = 0;
    start = get_time();
    for (int i = 0; i < SIZE; i++) {
        sum += a[i];
    }
    end = get_time();
    printf("Tempo soma cumulativa (1 var): %.6f segundos\n", end - start);
    printf("Valor da Soma: %d\n", sum);
    
    // 3. Soma com múltiplas variáveis (quebrando dependências)
    int sum1 = 0, sum2 = 0;
    start = get_time();
    for (int i = 0; i < SIZE; i += 2) {
        sum1 += a[i];
        sum2 += a[i+1];

    }
    sum = sum1 + sum2;
    end = get_time();
    printf("Tempo soma com 2 vars: %.6f segundos\n", end - start);
    printf("Valor da Soma: %d\n", sum);
    printf("-------------------------------------------------\n");

    return 0;
}
/*
gcc -o ilp_experiment_o0 -O0 ilp_experimento.c
gcc -o ilp_experiment_o2 -O2 ilp_experimento.c
gcc -o ilp_experiment_o3 -O3 ilp_experimento.c*/