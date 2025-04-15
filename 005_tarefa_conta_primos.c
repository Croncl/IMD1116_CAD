#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <time.h>

#define _POSIX_C_SOURCE 200809L

// Função para verificar primalidade (thread-safe)
bool is_prime(int num) {
    if (num <= 1) return false;
    if (num == 2) return true;
    if (num % 2 == 0) return false;
    
    for (int i = 3; i * i <= num; i += 2) {
        if (num % i == 0) return false;
    }
    return true;
}

// Versão sequencial
int count_primes_sequential(int max_num) {
    int count = 0;
    for (int i = 2; i <= max_num; i++) {
        if (is_prime(i)) count++;
    }
    return count;
}

// Versão paralela com OpenMP
int count_primes_parallel(int max_num) {
    int count = 0;
    #pragma omp parallel for reduction(+:count)
    for (int i = 2; i <= max_num; i++) {
        if (is_prime(i)) count++;
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <valor_maximo>\n", argv[0]);
        return 1;
    }

    const int max_num = atoi(argv[1]);
    if (max_num < 2) {
        printf("Erro: O valor deve ser ≥ 2\n");
        return 1;
    }

    struct timespec start, end;
    double seq_time, par_time;
    int seq_count, par_count;

    // ===== EXECUÇÃO SEQUENCIAL =====
    clock_gettime(CLOCK_MONOTONIC, &start);
    seq_count = count_primes_sequential(max_num);
    clock_gettime(CLOCK_MONOTONIC, &end);
    seq_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // ===== EXECUÇÃO PARALELA =====
    clock_gettime(CLOCK_MONOTONIC, &start);
    par_count = count_primes_parallel(max_num);
    clock_gettime(CLOCK_MONOTONIC, &end);
    par_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // ===== RESULTADOS =====
    printf("\n═══════════════════════════════════════════════\n");
    printf("           CONTAGEM DE NÚMEROS PRIMOS          \n");
    printf("═══════════════════════════════════════════════\n");
    printf("• Limite superior:      %d\n", max_num);
    printf("• Números primos encontrados: %d\n", seq_count);
    printf("\n─── TEMPOS DE EXECUÇÃO ────────────────────────\n");
    printf("Sequencial:  %.6f segundos\n", seq_time);
    printf("Paralelo:    %.6f segundos\n", par_time);
    printf("\n─── ANÁLISE DE DESEMPENHO ────────────────────\n");
    printf("Speedup:     %.2fx\n", seq_time / par_time);
    printf("Eficiência:  %.1f%%\n", 100 * (seq_time / par_time) / omp_get_max_threads());
    printf("═══════════════════════════════════════════════\n");
    printf("\nNota: O Speedup é calculado como o tempo sequencial dividido pelo tempo paralelo.\n");
    printf("      A Eficiência é o Speedup dividido pelo número de threads disponíveis.\n");

    // Validação dos resultados
    if (seq_count != par_count) {
        printf("⚠️  AVISO: Os resultados divergem (seq=%d vs par=%d)!\n", 
               seq_count, par_count);
    } else {
        printf("✓ Verificação: Resultados consistentes.\n");
    }

    return 0;
}

//gcc -o 005_tarefa_conta_primos 005_tarefa_conta_primos.c -lm -fopenmp