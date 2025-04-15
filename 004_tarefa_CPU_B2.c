#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <math.h>

#define ITERACOES 10000000  // Número de iterações externas
#define TERMS 20           // Termos para séries de Taylor
#define TRIES 5            // Número de repetições para média

// Função para calcular diferença de tempo em segundos com alta precisão
double timespec_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

// Cálculo complexo: Série de Taylor para sin(x) + cos(x) + exp(x)
double complex_calculation(double x) {
    double sin_x = 0.0, cos_x = 0.0, exp_x = 1.0;
    double x_power = x;
    double factorial = 1.0;
    
    // Série de Taylor para sin(x)
    for(int n = 1; n <= TERMS; n += 2) {
        sin_x += x_power / factorial;
        x_power *= -x * x;
        factorial *= (n + 1) * (n + 2);
    }
    
    // Reset para cos(x)
    x_power = x; // Correção: deve começar com x^1 para sin(x)
    factorial = 1.0;
    
    // Série de Taylor para cos(x)
    for(int n = 0; n <= TERMS; n += 2) {
        cos_x += x_power / factorial;
        x_power *= -x * x;
        factorial *= (n + 1) * (n + 2);
    }
    
    // Reset para exp(x)
    x_power = x; // Correção: deve começar com x^1
    factorial = 1.0;
    
    // Série de Taylor para exp(x)
    for(int n = 1; n <= TERMS; n++) {
        exp_x += x_power / factorial;
        x_power *= x;
        factorial *= n;
    }
    
    return sin_x + cos_x + exp_x;
}

void benchmark_compute_bound() {
    struct timespec start, end;
    volatile double result; // Prevenção contra otimização

    printf("\nBenchmark Compute-Bound (Cálculo Matemático Complexo)\n");
    printf("====================================================\n");
    printf("Iterações: %d\n", ITERACOES);
    printf("Termos nas séries: %d\n", TERMS);
    printf("Testes por configuração: %d\n", TRIES);
    printf("Tempos em segundos com 6 casas decimais\n\n");
    
    // Versão serial
    double serial_time = 0;
    for(int t = 0; t < TRIES; t++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        double sum = 0.0;
        for (int i = 0; i < ITERACOES; i++) {
            sum += complex_calculation(i * 0.0001); // Argumento variado
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        serial_time += timespec_diff(&start, &end);
        result = sum; // Uso forçado do resultado
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
            for (int i = 0; i < ITERACOES; i++) {
                sum += complex_calculation(i * 0.0001);
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_time += timespec_diff(&start, &end);
            result = sum; // Uso forçado do resultado
        }
        
        double avg_time = total_time / TRIES;
        double speedup = serial_time / avg_time;
        printf("%7d | %16.6f | %6.2fx\n", num_threads, avg_time, speedup);
    }
}

int main() {
    benchmark_compute_bound();
    return 0;
}
// compilar: gcc -o 004_tarefa_CPU_B2 004_tarefa_CPU_B2.c -fopenmp -lrt -lm