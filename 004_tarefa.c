#include <stdio.h>
#include <time.h>
#include <omp.h>    // Biblioteca para paralelismo com OpenMP
#include <math.h>   // Funções matemáticas (sin, cos, exp, sqrt)
#include <stdlib.h> // malloc, exit

// ==============================================
// CONFIGURAÇÕES GLOBAIS
// ==============================================
#define CPU_ITER 10000000    // 10 milhões de iterações para CPU-bound
#define ARRAY_SIZE 1000000000 // Tamanho do array (não utilizado neste código)
#define TRIES 5              // Número de repetições para média de tempo

// ==============================================
// FUNÇÃO DE TEMPO PRECISO (Linux)
// ==============================================
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Obtém tempo monotônico (não afetado por ajustes do sistema)
    return ts.tv_sec + ts.tv_nsec / 1e9; // Converte para segundos (parte inteira + fracionária)
}

// ==============================================
// CPU-BOUND: CÁLCULOS INTENSIVOS
// ==============================================
double cpu_task(double x) {
    double sum = 0;
    // Loop com operações matemáticas complexas para forçar uso intensivo da CPU
    for(int i = 0; i < 20; i++) {
        sum += sin(x) + cos(x) + exp(x); // Combinação custosa (transcendental + exponencial)
        x += 0.0001; // Pequeno incremento para variar os cálculos
    }
    return sum;
}

void cpu_bench() {
    printf("\n=== CPU-BOUND (Cálculos Intensivos) ===\n");
    
    // ------------------------------------------
    // EXECUÇÃO SERIAL (REFERÊNCIA)
    // ------------------------------------------
    double start = get_time();
    double serial_sum = 0;
    for(int i = 0; i < CPU_ITER; i++) {
        serial_sum += cpu_task(i * 0.001); // Acumula resultados
    }
    double serial_time = get_time() - start;
    
    printf("Serial: %.4f s\n", serial_time);
    printf("Threads | Tempo (s) | Speedup | Eficiencia\n");
    
    // ------------------------------------------
    // EXECUÇÃO PARALELA (1 a 4 THREADS)
    // ------------------------------------------
    for(int t = 1; t <= 4; t++) {
        double total_time = 0;
        double par_sum = 0;
        
        // Repete TRIES vezes para média estável
        for(int try = 0; try < TRIES; try++) {
            start = get_time();
            double sum = 0;
            // Diretiva OpenMP: paraleliza o loop com redução de soma
            #pragma omp parallel for reduction(+:sum) num_threads(t)
            for(int i = 0; i < CPU_ITER; i++) {
                sum += cpu_task(i * 0.001);
            }
            total_time += get_time() - start;
            par_sum += sum;
        }
        
        // Cálculo de métricas
        double avg_time = total_time / TRIES;
        double speedup = serial_time / avg_time; // Aceleração em relação ao serial
        double eff = (speedup / t) * 100;       // Eficiência (speedup ideal = t)
        
        printf("%7d | %9.4f | %7.2f | %8.2f%%\n", 
               t, avg_time, speedup, eff);
    }
}

// ==============================================
// MEMORY-BOUND: ACESSO INTENSIVO À MEMÓRIA
// ==============================================
void mem_bench() {
    const long ROWS = 20000;  // Tamanho da matriz (20000x20000)
    const long COLS = 20000;  // ~3GB para double (20000² * 8 bytes)
    
    // Alocação dinâmica da matriz
    double *matrix = malloc(ROWS * COLS * sizeof(double));
    if(matrix == NULL) {
        printf("Falha na alocação de %.1fMB\n", 
              (ROWS*COLS*sizeof(double))/(1024.0*1024.0));
        exit(1);
    }

    // ------------------------------------------
    // INICIALIZAÇÃO PARALELA (valores fictícios)
    // ------------------------------------------
    #pragma omp parallel for
    for(long i = 0; i < ROWS*COLS; i++) {
        matrix[i] = (i % 100) * 0.0001; // Padrão repetitivo para dados
    }

    printf("\n=== MEMORY-BOUND (Matriz %.1fMB) ===\n", 
          (ROWS*COLS*sizeof(double))/(1024.0*1024.0));

    // ------------------------------------------
    // BENCHMARK SERIAL (acesso aleatório)
    // ------------------------------------------
    double start = get_time();
    double sum = 0.0;
    for(long n = 0; n < ROWS*COLS; n++) {
        long i = (n * 32771) % (ROWS*COLS); // Pseudo-aleatoriedade (número primo)
        sum += sqrt(matrix[i] * matrix[i] + 0.5) * 1.0001; // Operação simples
    }
    double serial_time = get_time() - start;
    
    printf("Serial: %.4f s (sum=%.2f)\n", serial_time, sum);
    printf("Threads | Tempo (s) | Speedup | Eficiencia\n");
    printf("--------|-----------|---------|-----------\n");
    
    // ------------------------------------------
    // EXECUÇÃO PARALELA (1 a 4 THREADS)
    // ------------------------------------------
    for(int t = 1; t <= 4; t++) {
        double total_time = 0;
        double par_sum = 0;
        
        for(int try = 0; try < TRIES; try++) {
            start = get_time();
            double local_sum = 0;
            // Paralelização com acesso aleatório
            #pragma omp parallel for reduction(+:local_sum) num_threads(t)
            for(long n = 0; n < ROWS*COLS; n++) {
                long i = (n * 32771) % (ROWS*COLS);
                local_sum += sqrt(matrix[i] * matrix[i] + 0.5) * 1.0001;
            }
            total_time += get_time() - start;
            par_sum += local_sum;
        }
        
        double avg_time = total_time / TRIES;
        double speedup = serial_time / avg_time;
        double eff = (speedup / t) * 100;
        
        printf("%7d | %9.4f | %7.2f | %9.2f%%\n", 
               t, avg_time, speedup, eff);
    }
    
    free(matrix); // Liberação de memória
}

// ==============================================
// FUNÇÃO PRINCIPAL
// ==============================================

int main() {
    printf("====================================\n");
    printf(" CPU-BOUND vs MEMORY-BOUND\n");
    printf(" i5-3210M (2 núcleos, 4 threads)\n");
    printf("====================================\n");
    
    cpu_bench();
    mem_bench();

    // =========================================
    // ANÁLISE DETALHADA DOS RESULTADOS
    // =========================================
    printf("\n=== ANÁLISE DOS RESULTADOS ===\n");
    
    printf("\n1. DEFINIÇÕES FUNDAMENTAIS:\n");
    printf("   - SPEEDUP: Razão entre tempo serial e tempo paralelo\n");
    printf("     (Ex: Speedup 2x = o paralelo foi 2x mais rápido)\n");
    printf("   - EFICIÊNCIA: Porcentagem do speedup ideal alcançado\n");
    printf("     (100%% = todo recurso adicional foi bem aproveitado)\n");
    
    printf("\n2. COMPORTAMENTO CPU-BOUND:\n");
    printf("   - Espera-se bom scaling até 2 threads (núcleos físicos)\n");
    printf("   - Hyper-Threading (4 threads) pode trazer ganhos menores\n");
    printf("   - Eficiência cai quando threads competem por:\n");
    printf("     * Unidades de execução da CPU\n");
    printf("     * Recursos compartilhados (cache, FPU)\n");
    
    printf("\n3. COMPORTAMENTO MEMORY-BOUND:\n");
    printf("   - Scaling limitado pela banda de memória\n");
    printf("   - Pouco benefício além de 2 threads\n");
    printf("   - Eficiência cai mais rápido que CPU-bound porque:\n");
    printf("     * Várias threads saturam o barramento de memória\n");
    printf("     * Acesso à memória torna-se o gargalo principal\n");
    
    printf("\n4. COMPARAÇÃO DIRETA:\n");
    printf("   | Fator          | CPU-Bound | Memory-Bound |\n");
    printf("   |----------------|-----------|--------------|\n");
    printf("   | Melhora com HT | Sim       | Pouco        |\n");
    printf("   | Gargalo        | CPU       | Memória      |\n");
    printf("   | Threads ótimas | 2-4       | 1-2          |\n");
    
    printf("\n5. CONCLUSÃO:\n");
    printf("   - CPU-bound: Paralelismo ajuda, mas com retornos decrescentes\n");
    printf("   - Memory-bound: Paralelismo tem benefícios limitados\n");
    printf("   - Hyper-Threading é mais útil em cargas CPU-intensive\n");
    
    return 0;
}
// COMPILAÇÃO:
// gcc -o 004_tarefa 004_tarefa.c -fopenmp -lm -lrt -O0
// -fopenmp: Habilita OpenMP
// -lm: Linka math.h
// -lrt: Linka clock_gettime (Linux)
// -O0: Desativa otimizações para medição mais precisa