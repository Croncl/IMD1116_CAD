#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <string.h>

// Dimensões da malha 3D
#define NX 10
#define NY 10
#define NZ 10

// Número de passos da simulação
#define NSTEPS 1000

// Constantes físicas
const double nu = 0.1;
const double dt = 0.01;
const double dx = 1.0;

// Conversão de índice 3D para 1D
int idx(int i, int j, int k) {
    return i * NY * NZ + j * NZ + k;
}

// Inicializa a malha com um pico no centro
void initialize(double *u) {
    for (int i = 0; i < NX; ++i)
        for (int j = 0; j < NY; ++j)
            for (int k = 0; k < NZ; ++k) {
                if ((i > NX / 2 - 2 && i < NX / 2 + 2) &&
                    (j > NY / 2 - 2 && j < NY / 2 + 2) &&
                    (k > NZ / 2 - 2 && k < NZ / 2 + 2))
                    u[idx(i, j, k)] = 1.0;
                else
                    u[idx(i, j, k)] = 0.0;
            }
}

// Estrutura auxiliar para armazenar tempo e valor central por passo
typedef struct {
    double tempo;
    double valor_central;
} ResultadoPasso;

// Array 3D: [passo][afinidade][threads]
ResultadoPasso resultados_por_passo[NSTEPS][5][3];

// Executa a simulação para uma configuração
double run_simulation(const char *affinity_type, int thread_count, int affinity_index, int thread_index) {
    int size = NX * NY * NZ;
    double *u = (double*) malloc(size * sizeof(double));
    double *u_new = (double*) malloc(size * sizeof(double));
    if (!u || !u_new) {
        fprintf(stderr, "Erro de alocação de memória\n");
        exit(1);
    }

    initialize(u);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int n = 0; n < NSTEPS; ++n) {
        struct timespec start_step, end_step;
        clock_gettime(CLOCK_MONOTONIC, &start_step);

        #pragma omp parallel for collapse(3) proc_bind(spread) schedule(static)
        for (int i = 1; i < NX - 1; ++i)
            for (int j = 1; j < NY - 1; ++j)
                for (int k = 1; k < NZ - 1; ++k) {
                    double laplacian =
                        (u[idx(i + 1, j, k)] + u[idx(i - 1, j, k)] +
                         u[idx(i, j + 1, k)] + u[idx(i, j - 1, k)] +
                         u[idx(i, j, k + 1)] + u[idx(i, j, k - 1)] -
                         6.0 * u[idx(i, j, k)]) / (dx * dx);

                    u_new[idx(i, j, k)] = u[idx(i, j, k)] + dt * nu * laplacian;
                }

        clock_gettime(CLOCK_MONOTONIC, &end_step);
        double elapsed_step = (end_step.tv_sec - start_step.tv_sec) +
                             (end_step.tv_nsec - start_step.tv_nsec) / 1e9;

        double *tmp = u;
        u = u_new;
        u_new = tmp;

        double center_value = u[idx(NX/2, NY/2, NZ/2)];

        // Armazena os dados do passo na estrutura auxiliar
        resultados_por_passo[n][affinity_index][thread_index].tempo = elapsed_step;
        resultados_por_passo[n][affinity_index][thread_index].valor_central = center_value;

        if (n % 100 == 0) {
            printf("Passo %d/%d | %s (%d threads) | Valor: %.4f | Tempo: %.3f s\n",
                   n, NSTEPS, affinity_type, thread_count, center_value, elapsed_step);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    free(u);
    free(u_new);
    return elapsed;
}

// Define afinidade OpenMP
void set_affinity(const char *affinity_type) {
    if (strcmp(affinity_type, "false") == 0) {
        setenv("OMP_PROC_BIND", "false", 1);
        setenv("OMP_PLACES", "", 1);
    } 
    else if (strcmp(affinity_type, "true") == 0) {
        setenv("OMP_PROC_BIND", "true", 1);
        setenv("OMP_PLACES", "cores", 1);
    }
    else if (strcmp(affinity_type, "close") == 0 ||
             strcmp(affinity_type, "spread") == 0 ||
             strcmp(affinity_type, "master") == 0) {
        setenv("OMP_PROC_BIND", affinity_type, 1);
        setenv("OMP_PLACES", "cores", 1);
    }
}

// Exibe tabela com tempos totais
void print_results_table(double results[][5], const char *affinity_types[], int num_types, int thread_counts[], int num_thread_counts) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║               RESULTADOS DE DESEMPENHO                  ║\n");
    printf("╠════════════╦══════════╦══════════╦══════════╦══════════╣\n");
    printf("║ Afinidade  ║   32T    ║   64T    ║  128T    ║ Speedup  ║\n");
    printf("╠════════════╬══════════╬══════════╬══════════╬══════════╣\n");

    for (int i = 0; i < num_types; i++) {
        printf("║ %-10s ", affinity_types[i]);
        for (int t = 0; t < num_thread_counts; t++) {
            printf("║ %8.2f ", results[t][i]);
        }
        double speedup = results[0][0]/results[num_thread_counts-1][i];
        printf("║ %8.2f ║\n", speedup);

        if (i < num_types-1) {
            printf("╟────────────╫──────────╫──────────╫──────────╫──────────╢\n");
        }
    }
    printf("╚════════════╩══════════╩══════════╩══════════╩══════════╝\n");
}

int main() {
    const char *affinity_types[] = {"false", "true", "close", "spread", "master"};
    int num_types = sizeof(affinity_types) / sizeof(affinity_types[0]);
    int thread_counts[] = {32, 64, 128};
    int num_thread_counts = sizeof(thread_counts) / sizeof(thread_counts[0]);

    double results[num_thread_counts][num_types];

    printf("Iniciando simulações...\n");

    for (int t = 0; t < num_thread_counts; t++) {
        omp_set_num_threads(thread_counts[t]);
        printf("\n=== CONFIGURAÇÃO COM %d THREADS ===\n", thread_counts[t]);

        for (int i = 0; i < num_types; i++) {
            set_affinity(affinity_types[i]);
            printf("\nExecutando com afinidade '%s'...\n", affinity_types[i]);
            results[t][i] = run_simulation(affinity_types[i], thread_counts[t], i, t);
            printf("Tempo total: %.2f segundos\n", results[t][i]);
        }
    }

    // Cria CSV com cabeçalho dinâmico
    char nome_completo[100];
    sprintf(nome_completo, "013_resultados_completosN%d.csv", NX);
    FILE *fp = fopen(nome_completo, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar arquivo CSV\n");
        exit(1);
    }

    // Escreve cabeçalho
    fprintf(fp, "Passo,ValorCentral");
    for (int i = 0; i < num_types; i++) {
        for (int t = 0; t < num_thread_counts; t++) {
            fprintf(fp, ",Tempo_%s_%d", affinity_types[i], thread_counts[t]);
        }
    }
    fprintf(fp, "\n");

    // Escreve dados por linha de passo
    for (int n = 0; n < NSTEPS; n++) {
        // Assume que o valor central pode ser o primeiro registrado (ex: false + 32)
        double valor_central = resultados_por_passo[n][0][0].valor_central;
        fprintf(fp, "%d,%.5f", n, valor_central);
        for (int i = 0; i < num_types; i++) {
            for (int t = 0; t < num_thread_counts; t++) {
                fprintf(fp, ",%.6f", resultados_por_passo[n][i][t].tempo);
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("\nDados organizados salvos em '%s.csv'\n", nome_completo);

    // Salva resumo final
    char nome_resumo[100];
    sprintf(nome_resumo, "013_resultados_resumoN%d.csv", NX);
    FILE *fp_resumo = fopen(nome_resumo, "w");
    if (fp_resumo) {
        fprintf(fp_resumo, "Afinidade,32_threads,64_threads,128_threads,Speedup\n");
        for (int i = 0; i < num_types; i++) {
            fprintf(fp_resumo, "%s", affinity_types[i]);
            for (int t = 0; t < num_thread_counts; t++) {
                fprintf(fp_resumo, ",%.2f", results[t][i]);
            }
            double speedup = results[0][0]/results[num_thread_counts-1][i];
            fprintf(fp_resumo, ",%.2f\n", speedup);
        }
        fclose(fp_resumo);
        printf("Resumo salvo em '%s.csv'\n", nome_resumo);
    }

    // Exibe resumo
    print_results_table(results, affinity_types, num_types, thread_counts, num_thread_counts);

    return 0;
}

/*
#!/bin/bash
#SBATCH --time=0-0:10
#SBATCH --partition=amd-512
#SBATCH --cpus-per-task=128
#SBATCH --job-name=013_tarefa_job
#SBATCH --output=013_tarefa_%j.out
#SBATCH --error=013_tarefa_%j.err

module load compilers/gnu/14.2.0

# Exporta variáveis para OpenMP
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PLACES=cores

# Compila o código
gcc -fopenmp 013_tarefa.c -o 013_tarefa -lm

# Executa a simulação
./013_tarefa


*/