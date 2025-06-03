#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <string.h>

// Dimensões da malha 3D
#define NX 100
#define NY 100
#define NZ 100

// Número de passos da simulação
#define NSTEPS 2000

// Constantes físicas e de discretização
const double nu = 0.1;     // Viscosidade
const double dt = 0.01;    // Passo de tempo
const double dx = 1.0;     // Espaçamento entre pontos

// Converte índice 3D (i,j,k) para índice linear 1D
int idx(int i, int j, int k) {
    return i * NY * NZ + j * NZ + k;
}

// Inicializa o campo u com um pico central no domínio
void initialize(double *u) {
    #pragma omp parallel for collapse(3)
    for (int i = 0; i < NX; ++i)
        for (int j = 0; j < NY; ++j)
            for (int k = 0; k < NZ; ++k) {
                if ((i > NX / 2 - 2 && i < NX / 2 + 2) &&
                    (j > NY / 2 - 2 && j < NY / 2 + 2) &&
                    (k > NZ / 2 - 2 && k < NZ / 2 + 2))
                    u[idx(i, j, k)] = 1.0;  // Pico central
                else
                    u[idx(i, j, k)] = 0.0;  // Zero no restante do domínio
            }
}

/**
 * Função que executa a simulação da equação de difusão,
 * registrando valor central e tempo de cada passo no mesmo arquivo CSV.
 * 
 * Parâmetros:
 *   u, u_new: vetores do campo atual e próximo
 *   fp: arquivo aberto para escrita CSV (Passo,ValorCentral,Tempo)
 *   schedule_type: string indicando o tipo de agendamento OpenMP
 *   chunk_size: tamanho do chunk para schedule dynamic
 */
void simulate(double *u, double *u_new, FILE *fp, const char *schedule_type, int chunk_size) {
    struct timespec start_step, end_step;

    for (int n = 0; n < NSTEPS; ++n) {
        // Marca tempo de início do passo
        clock_gettime(CLOCK_MONOTONIC, &start_step);

        // Atualiza os pontos internos da malha com paralelização OpenMP
        if (strcmp(schedule_type, "dynamic128") == 0) {
            #pragma omp parallel for collapse(3) schedule(dynamic, 128)
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
        } else if (strcmp(schedule_type, "dynamic64") == 0) {
            #pragma omp parallel for collapse(3) schedule(dynamic, 64)
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
        } else if (strcmp(schedule_type, "dynamic32") == 0) {
            #pragma omp parallel for collapse(3) schedule(dynamic, 32)
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
        } else {
            fprintf(stderr, "Tipo de agendamento desconhecido: %s\n", schedule_type);
            exit(1);
        }

        // Marca tempo de término do passo
        clock_gettime(CLOCK_MONOTONIC, &end_step);

        // Calcula o tempo gasto no passo
        double elapsed_step = (end_step.tv_sec - start_step.tv_sec) +
                              (end_step.tv_nsec - start_step.tv_nsec) / 1e9;

        // Troca os ponteiros dos vetores para próximo passo
        double *tmp = u;
        u = u_new;
        u_new = tmp;

        // Escreve no CSV: passo, valor no centro, tempo do passo
        double center_value = u[idx(NX / 2, NY / 2, NZ / 2)];
        fprintf(fp, "%d,%.5f,%.9f\n", n, center_value, elapsed_step);

        // Opcional: imprime no console a cada 200 passos
        if (n % 200 == 0) {
            printf("[%s] Passo %d | Valor central: %.5f | Tempo passo: %.6f s\n",
                   schedule_type, n, center_value, elapsed_step);
        }
    }
}

/**
 * Executa a simulação para um tipo específico de agendamento e retorna
 * o tempo total de execução.
 */
double run_simulation(const char *label, int chunk_size) {
    int size = NX * NY * NZ;

    // Aloca memória para os campos u e u_new
    double *u = (double*) malloc(size * sizeof(double));
    double *u_new = (double*) malloc(size * sizeof(double));
    if (!u || !u_new) {
        fprintf(stderr, "Erro de alocação de memória\n");
        exit(1);
    }

    // Inicializa o campo u
    initialize(u);

    // Abre arquivo CSV para gravar passo, valor central e tempo do passo
    char filename[64];
    snprintf(filename, sizeof(filename), "simulacaoNPAD_%s.csv", label);
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir arquivo %s\n", filename);
        exit(1);
    }

    // Escreve o cabeçalho do CSV
    fprintf(fp, "Passo,ValorCentral,Tempo\n");

    // Mede o tempo total da simulação
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Executa a simulação
    simulate(u, u_new, fp, label, chunk_size);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calcula o tempo total decorrido
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Tempo total [%s]: %.3f segundos\n", label, elapsed);

    // Libera memória e fecha arquivo
    free(u);
    free(u_new);
    fclose(fp);

    return elapsed;
}

int main() {
    printf("Iniciando comparação de schedule(dynamic) com diferentes chunk sizes:\n");

    // Executa as três variações de dynamic
    double t_dynamic128 = run_simulation("dynamic128", 128);
    double t_dynamic64 = run_simulation("dynamic64", 64);
    double t_dynamic32 = run_simulation("dynamic32", 32);

    printf("\nResumo dos tempos totais:\n");
    printf("Dynamic (chunk=128): %.3f s\n", t_dynamic128);
    printf("Dynamic (chunk=64) : %.3f s\n", t_dynamic64);
    printf("Dynamic (chunk=32) : %.3f s\n", t_dynamic32);

    // Calcula o speedup relativo entre as versões
    printf("\nComparação de desempenho:\n");
    printf("Speedup (128 vs 64): %.2fx\n", t_dynamic64 / t_dynamic128);
    printf("Speedup (128 vs 32): %.2fx\n", t_dynamic32 / t_dynamic128);
    printf("Speedup (64 vs 32) : %.2fx\n", t_dynamic32 / t_dynamic64);

    return 0;
}//gcc -fopenmp 012_tarefa_dynamic.c -o 012_tarefa_dynamic_12 -lm