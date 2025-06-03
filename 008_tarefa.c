#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <sys/time.h>

#define NUM_POINTS 100000000
#define PI_REF 3.14159265358979323846

double get_real_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void print_result(const char* name, double pi_estimate, double elapsed_time) {
    double error = fabs(pi_estimate - PI_REF);
    int correct_digits = (int)(-log10(error));
    
    printf("%s:\n", name);
    printf("  π estimado: %.8f\n", pi_estimate);
    printf("  Erro: %.2e\n", error);
    printf("  Dígitos corretos: %d\n", correct_digits);
    printf("  Tempo real: %.4f segundos\n\n", elapsed_time);
}

// Versão 1: rand + critical
double version1_rand_critical() {
    int points_inside = 0;
    
    #pragma omp parallel
    {
        double x, y;
        int local_inside = 0;
        
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand() / RAND_MAX;
            y = (double)rand() / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                local_inside++;
            }
        }
        
        #pragma omp critical
        points_inside += local_inside;
    }
    
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 2: rand + array (sem prevenção de falso compartilhamento)
double version2_rand_array() {
    int num_threads = omp_get_max_threads();
    int *points_array = (int*)calloc(num_threads, sizeof(int));
    int points_inside = 0;
    
    #pragma omp parallel
    {
        double x, y;
        int tid = omp_get_thread_num();
        
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand() / RAND_MAX;
            y = (double)rand() / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                points_array[tid]++;
            }
        }
    }
    
    for (int i = 0; i < num_threads; i++) {
        points_inside += points_array[i];
    }
    
    free(points_array);
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 3: rand_r + critical
double version3_randr_critical() {
    int points_inside = 0;
    int base_seed = time(NULL);
    
    #pragma omp parallel firstprivate(base_seed)
    {
        double x, y;
        int local_inside = 0;
        unsigned int seed = base_seed + omp_get_thread_num();
        
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;
            y = (double)rand_r(&seed) / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                local_inside++;
            }
        }
        
        #pragma omp critical
        points_inside += local_inside;
    }
    
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 4: rand_r + array (sem prevenção de falso compartilhamento)
double version4_randr_array() {
    int num_threads = omp_get_max_threads();
    int *points_array = (int*)calloc(num_threads, sizeof(int));
    int points_inside = 0;
    int base_seed = time(NULL);
    
    #pragma omp parallel firstprivate(base_seed)
    {
        double x, y;
        int tid = omp_get_thread_num();
        unsigned int seed = base_seed + tid;
        
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;
            y = (double)rand_r(&seed) / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                points_array[tid]++;
            }
        }
    }
    
    for (int i = 0; i < num_threads; i++) {
        points_inside += points_array[i];
    }
    
    free(points_array);
    return 4.0 * points_inside / NUM_POINTS;
}

int main() {
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("      ESTIMAÇÃO ESTOCÁSTICA DE π COM OPENMP - COMPARAÇÃO\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("Número de pontos: %d\n", NUM_POINTS);
    printf("Threads disponíveis: %d\n\n", omp_get_max_threads());

    double start, end, pi_estimate;
    
    // Versão 1: rand() com critical
    start = get_real_time();
    pi_estimate = version1_rand_critical();
    end = get_real_time();
    print_result("1. rand() com critical", pi_estimate, end - start);
    
    // Versão 2: rand() com array
    start = get_real_time();
    pi_estimate = version2_rand_array();
    end = get_real_time();
    print_result("2. rand() com array", pi_estimate, end - start);
    
    // Versão 3: rand_r() com critical
    start = get_real_time();
    pi_estimate = version3_randr_critical();
    end = get_real_time();
    print_result("3. rand_r() com critical", pi_estimate, end - start);
    
    // Versão 4: rand_r() com array
    start = get_real_time();
    pi_estimate = version4_randr_array();
    end = get_real_time();
    print_result("4. rand_r() com array", pi_estimate, end - start);

    return 0;
}
//gcc -fopenmp 008_tarefa.c -o 008_tarefa -lm


/*
Mofdificasr para(sao 4 versoes, sendo 2 com rand(), e 2 com rand_r()), comparadas com medicao de tempo de forma organizada e estruturada):
Cada thread deve usar uma variavel privada para contar os acertos e acumular o total em uma variavel global #pragma omp critical

- depois, implementare uma segunda versao em que cada theread escreve seus acertos em uma posicao distinda de um vetor compartilhado, a acumulacao deve ser feita em um laco serial apos a regiao paralela.ADJ_ESTERROR

- Compare o tempo de execucao das duas versoes
- em seguida, substitua rand() por rand_r() em ambas e compare novamente.ADJ_ESTERROR

-explique o comportamento dos 4 programas(versoes) com base na coerencia de cache nos efeitos do falso compartilhamento.


*/