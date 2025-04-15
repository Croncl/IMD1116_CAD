#include <stdio.h>
#include <time.h>
#include <omp.h>

#define SIZE 100000000  // 100 milhões de elementos (~800MB)
#define TRIES 5

// Array estático global (pode ser grande demais para a stack)
static double array[SIZE];

double timespec_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

void initialize_array() {
    for(int i = 0; i < SIZE; i++) {
        array[i] = (double)i / 1000.0;
    }
}

void benchmark_memory_bound() {
    initialize_array();
    
    printf("Benchmark Memory-Bound (Soma de Array)\n");
    printf("=====================================\n");
    printf("Tamanho do array: %.1f MB\n", (SIZE * sizeof(double)) / (1024.0 * 1024.0));
    printf("Testes por configuração: %d\n", TRIES);
    printf("Tempos em segundos com 6 casas decimais\n\n");
    
    struct timespec start, end;
    volatile double total_sum = 0; // Prevenção contra otimização
    
    // Versão serial
    double serial_time = 0;
    for(int t = 0; t < TRIES; t++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        double sum = 0.0;
        for(int i = 0; i < SIZE; i++) {
            sum += array[i];
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        serial_time += timespec_diff(&start, &end);
        total_sum += sum;
    }
    printf("Serial: %.6f segundos (média)\n", serial_time / TRIES);

    // Testar com diferentes números de threads
    int thread_counts[] = {1, 2, 3, 4, 8};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);

    printf("\nThreads | Tempo (s)        | Speedup\n");
    printf("--------|------------------|--------\n");

    for (int t = 0; t < num_tests; t++) {
        int num_threads = thread_counts[t];
        double total_time = 0;
        
        for(int trial = 0; trial < TRIES; trial++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            double sum = 0.0;
            #pragma omp parallel for reduction(+:sum) num_threads(num_threads)
            for(int i = 0; i < SIZE; i++) {
                sum += array[i];
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_time += timespec_diff(&start, &end);
            total_sum += sum;
        }
        
        double avg_time = total_time / TRIES;
        double speedup = serial_time / avg_time;
        printf("%7d | %16.6f | %6.2fx\n", num_threads, avg_time, speedup);
    }
}

int main() {
    benchmark_memory_bound();
    return 0;
}
//gcc -o 004_tarefa_memory_B 004_tarefa_memory_B.c -fopenmp -lrt