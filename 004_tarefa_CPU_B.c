#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string.h>

#define ITERACOES 1000000
#define TRIES 5

// Função para calcular diferença de tempo em segundos com alta precisão
double timespec_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

void calcular_sha(const unsigned char *input, size_t length, unsigned char *output) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input, length);
    EVP_DigestFinal_ex(ctx, output, NULL);
    EVP_MD_CTX_free(ctx);
}

void benchmark_compute_bound() {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const char* data = "dados_para_criptografia";
    struct timespec start, end;

    printf("\nBenchmark Compute-Bound (Criptografia SHA-256)\n");
    printf("=============================================\n");
    printf("Iterações: %d\n", ITERACOES);
    printf("Testes por configuração: %d\n", TRIES);
    printf("Tempos em segundos com 6 casas decimais\n\n");
    
    // Versão serial
    double serial_time = 0;
    for(int t = 0; t < TRIES; t++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < ITERACOES; i++) {
            calcular_sha(data, strlen(data), hash);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        serial_time += timespec_diff(&start, &end);
    }
    printf("Serial: %.6f segundos (média)\n", serial_time / TRIES);

    // Testar com diferentes números de threads
    int thread_counts[] = {1, 2, 4, 8, 16};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);

    printf("\nThreads | Tempo (s)        | Speedup\n");
    printf("--------|------------------|--------\n");

    for (int t = 0; t < num_tests; t++) {
        int num_threads = thread_counts[t];
        double total_time = 0;
        
        for(int trial = 0; trial < TRIES; trial++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            #pragma omp parallel for num_threads(num_threads)
            for (int i = 0; i < ITERACOES; i++) {
                calcular_sha(data, strlen(data), hash);
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_time += timespec_diff(&start, &end);
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
// compilar: gcc -o 004_tarefa_CPU_B 004_tarefa_CPU_B.c -lcrypto -fopenmp -O2 -lrt