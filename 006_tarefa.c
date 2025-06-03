#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>

#define NUM_POINTS 100000000  // 100 milhões de pontos
#define PI_REF 3.14159265358979323846

// Versão 1: Implementação paralela incorreta com condição de corrida
double version_race_condition() {
    int points_inside = 0;  // Variável compartilhada
    
    #pragma omp parallel for  // Paralelização simples
    for (int i = 0; i < NUM_POINTS; i++) {
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;
        
        // PROBLEMA: múltiplas threads acessando points_inside simultaneamente
        if (x*x + y*y <= 1.0) {
            points_inside++; // Condição de corrida ocorre aqui
        }
    }
    
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 2: Correção com critical e private
double version_critical_fix() {
    int points_inside = 0;  // Variável compartilhada
    
    #pragma omp parallel  // Região paralela
    {
        // Variáveis privadas para cada thread
        double x, y;
        int local_inside = 0;
        
        #pragma omp for private(x, y)  // Loop paralelo com variáveis privadas
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand() / RAND_MAX;
            y = (double)rand() / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                local_inside++;  // Contagem local sem condição de corrida
            }
        }
        
        // Seção crítica para atualizar o contador global
        #pragma omp critical
        points_inside += local_inside;
    }
    
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 3: Uso de firstprivate para sementes aleatórias
double version_firstprivate_seed() {
    int points_inside = 0;
    int base_seed = time(NULL);  // Semente base
    
    #pragma omp parallel firstprivate(base_seed)  // Cada thread recebe sua cópia
    {
        double x, y;
        int local_inside = 0;
        unsigned int seed = base_seed + omp_get_thread_num();  // Semente única por thread
        
        #pragma omp for private(x, y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand_r(&seed) / RAND_MAX;  // rand_r é thread-safe
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
// Versão 4: Demonstração de lastprivate
double version_lastprivate_demo() {
    int points_inside = 0;
    double last_x = 0, last_y = 0;  // Serão atualizadas pela última iteração
    
    #pragma omp parallel
    {
        double x, y;
        int local_inside = 0;
        // lastprivate preserva os valores da última iteração
        #pragma omp for private(x, y) lastprivate(last_x, last_y)
        for (int i = 0; i < NUM_POINTS; i++) {
            x = (double)rand() / RAND_MAX;
            y = (double)rand() / RAND_MAX;
            
            if (x*x + y*y <= 1.0) {
                local_inside++;
            }
            // Apenas a última iteração atualiza last_x e last_y
            if (i == NUM_POINTS - 1) {
                last_x = x;
                last_y = y;
            }
        }
        #pragma omp critical
        points_inside += local_inside;
    }
    printf("Último ponto gerado: (%.6f, %.6f)\n", last_x, last_y);
    return 4.0 * points_inside / NUM_POINTS;
}

// Versão 5: Uso de default(none) para clareza
double version_default_none() {
    int points_inside = 0;  // shared
    int base_seed = time(NULL);  // shared
    
    #pragma omp parallel default(none) shared(points_inside, base_seed)
    {
        // Todas as variáveis devem ser explicitamente declaradas
        double x, y;  // private
        int local_inside = 0;  // private
        unsigned int seed = base_seed + omp_get_thread_num();  // private
        
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

// Função auxiliar para imprimir resultados
void print_result(const char* name, double pi_estimate) {
    double error = fabs(pi_estimate - PI_REF);
    int correct_digits = (int)(-log10(error));
    
    printf("%s:\n", name);
    printf("  π estimado: %.8f\n", pi_estimate);
    printf("  Erro: %.2e\n", error);
    printf("  Dígitos corretos: %d\n\n", correct_digits);
}

int main() {
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("      ESTIMAÇÃO ESTOCÁSTICA DE π COM OPENMP - DEMONSTRAÇÃO\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("Número de pontos: %d\n", NUM_POINTS);
    printf("Threads disponíveis: %d\n\n", omp_get_max_threads());

    // Versão com condição de corrida (resultado incorreto)
    print_result("1. Paralelização ingênua (com condição de corrida)", 
                version_race_condition());
    
    // Versão corrigida com critical
    print_result("2. Correção com private e critical", 
                version_critical_fix());
    
    // Versão com firstprivate para sementes
    print_result("3. Uso de firstprivate para sementes aleatórias", 
                version_firstprivate_seed());
    
    // Versão demonstrando lastprivate
    print_result("4. Demonstração de lastprivate", 
                version_lastprivate_demo());
    
    // Versão com default(none)
    print_result("5. Uso de default(none) para clareza", 
                version_default_none());

    return 0;
}
//gcc -fopenmp 006_tarefa.c -o 006_tarefa -lm
