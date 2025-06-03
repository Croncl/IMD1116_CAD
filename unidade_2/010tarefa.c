#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>

// Número total de pontos a serem gerados para a estimativa de π
#define NUM_POINTS 1000000000
#define PI_REF 3.14159265358979323846 // Valor de referência de π com alta precisão

// Função que retorna o tempo real (em segundos)
double get_real_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Função que compara digito a digito a parte decimal de pi com a referência
int count_correct_digits(double pi_estimate) {
    // Buffers com 20 caracteres para armazenar os números com 15 casas decimais
    char ref[32], est[32];
    
    // Converte os números para string com 15 casas decimais
    sprintf(ref, "%.15f", PI_REF);
    sprintf(est, "%.15f", pi_estimate);

    int i = 0;

    // Pula a parte inteira (até o ponto)
    while (ref[i] && ref[i] == est[i]) {
        i++;
    }

    // Subtrai 2 para desconsiderar "3."
    return i - 2;
}


// Função que imprime o resultado da estimativa
void print_result(const char* name, double pi_estimate, double elapsed_time) {
    int correct_digits = count_correct_digits(pi_estimate);

    printf("%s:\n", name);
    printf(" π estimado: %.15f\n", pi_estimate);
    printf(" Dígitos corretos(parte decimal): %d\n", correct_digits);
    printf(" Tempo real: %.4f segundos\n\n", elapsed_time);
}

// Versão 1: usa #pragma omp critical para sincronizar acesso à variável compartilhada
double version_critical() {
    int points_inside = 0;
    int base_seed = time(NULL); // Semente base para rand_r

    #pragma omp parallel
    {
        double x, y;
        int local_inside = 0;
        unsigned int seed = base_seed + omp_get_thread_num(); // Semente única por thread

        // Distribuição das iterações entre as threads
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;
            y = (double)rand_r(&seed) / RAND_MAX;

            if (x*x + y*y <= 1.0) {
                // Incrementa contador local
                local_inside++;
            }
        }

        // Atualiza o contador global de forma protegida por seção crítica
        #pragma omp critical
        points_inside += local_inside;
    }
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 2: usa #pragma omp atomic para sincronizar incrementos ao contador global
double version_atomic() {
    int points_inside = 0;
    int base_seed = time(NULL);

    #pragma omp parallel
    {
        double x, y;
        unsigned int seed = base_seed + omp_get_thread_num();

        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;
            y = (double)rand_r(&seed) / RAND_MAX;

            if (x*x + y*y <= 1.0) {
                // Incrementa diretamente o contador global com atomic
                #pragma omp atomic
                points_inside++;
            }
        }
    }
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 3: usa #pragma omp reduction para somar automaticamente os valores de cada thread
double version_reduction() {
    int base_seed = time(NULL);
    int points_inside = 0;

    // A cláusula reduction já cuida da soma entre as threads
    #pragma omp parallel reduction(+:points_inside)
    {
        double x, y;
        unsigned int seed = base_seed + omp_get_thread_num();

        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;
            y = (double)rand_r(&seed) / RAND_MAX;

            if (x*x + y*y <= 1.0) {
                points_inside++;
            }
        }
    }
    return 4.0 * points_inside / NUM_POINTS;
}

// Função principal
int main() {
    printf("══════════════════════════════════════════════════════════════════\n");
    printf(" ESTIMAÇÃO DE π - COMPARAÇÃO DE MECANISMOS DE SINCRONIZAÇÃO\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("Número de pontos: %d\n", NUM_POINTS);
    printf("Threads disponíveis: %d\n\n", omp_get_max_threads());

    double start, end, pi_estimate;

    // Versão 1 - critical
    start = get_real_time();
    pi_estimate = version_critical();
    end = get_real_time();
    print_result("1. rand_r() com critical", pi_estimate, end - start);

    // Versão 2 - atomic
    start = get_real_time();
    pi_estimate = version_atomic();
    end = get_real_time();
    print_result("2. rand_r() com atomic", pi_estimate, end - start);

    // Versão 3 - reduction
    start = get_real_time();
    pi_estimate = version_reduction();
    end = get_real_time();
    print_result("3. rand_r() com reduction", pi_estimate, end - start);

    return 0;
}


//gcc -fopenmp 010tarefa.c -o 010tarefa -lm