#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

#define PI acos(-1.0)
#define LINHA "-----------------------------------------------------------------\n"

// Function declarations
void initial_value(const int n, const double dx, const double length, double * restrict u) {
    double y = dx;
    for (int j = 0; j < n; ++j) {
        double x = dx;
        for (int i = 0; i < n; ++i) {
            u[i + j * n] = sin(PI * x / length) * sin(PI * y / length);
            x += dx;
        }
        y += dx;
    }
}

void zero(const int n, double * restrict u) {
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            u[i + j * n] = 0.0;
        }
    }
}

void solve(const int n, const double alpha, const double dx, const double dt, 
           const double * restrict u, double * restrict u_tmp) {
    const double r = alpha * dt / (dx * dx);
    const double r2 = 1.0 - 4.0 * r;

    #pragma omp target teams distribute parallel for collapse(2) \
        map(to: u[0:n*n]) map(from: u_tmp[0:n*n])
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            u_tmp[i + j * n] = r2 * u[i + j * n] +
                               r * ((i < n - 1) ? u[(i + 1) + j * n] : 0.0) +
                               r * ((i > 0)     ? u[(i - 1) + j * n] : 0.0) +
                               r * ((j < n - 1) ? u[i + (j + 1) * n] : 0.0) +
                               r * ((j > 0)     ? u[i + (j - 1) * n] : 0.0);
        }
    }
}

double solution(const double t, const double x, const double y, 
               const double alpha, const double length) {
    return exp(-2.0 * alpha * PI * PI * t / (length * length)) *
           sin(PI * x / length) * sin(PI * y / length);
}

double l2norm(const int n, const double * restrict u, const int nsteps, 
              const double dt, const double alpha, const double dx, const double length) {
    double time = dt * (double)nsteps;
    double norm = 0.0;
    double y = dx;

    for (int j = 0; j < n; ++j) {
        double x = dx;
        for (int i = 0; i < n; ++i) {
            double ans = solution(time, x, y, alpha, length);
            double diff = u[i + j * n] - ans;
            norm += diff * diff;
            x += dx;
        }
        y += dx;
    }
    return sqrt(norm);
}

int main(int argc, char *argv[]) {
    printf("\n=== SOLUÇÃO DA EQUAÇÃO DO CALOR ===\n");
    printf("=== Paralelização em GPU com OpenMP ===\n\n");

    double inicio = omp_get_wtime();
    int n = 1000;
    int npassos = 10;

    if (argc == 3) {
        n = atoi(argv[1]);
        npassos = atoi(argv[2]);
        if (n <= 0 || npassos <= 0) {
            fprintf(stderr, "Erro: n e npassos devem ser positivos\n");
            exit(EXIT_FAILURE);
        }
    }

    double alpha = 0.1;
    double comprimento = 1000.0;
    double dx = comprimento / (n + 1);
    double dt = 0.5 / npassos;
    double r = alpha * dt / (dx * dx);

    printf(LINHA);
    printf("CONFIGURAÇÃO DO PROBLEMA:\n");
    printf(" Tamanho da grade:    %d x %d (%d milhões de células)\n", n, n, (n * n) / 1000000);
    printf(" Passos de tempo:     %d\n", npassos);
    printf(" Tempo de simulação:  %E segundos\n", dt * (double)npassos);
    printf(" Largura da célula:   %E\n", dx);
    printf(" Passo de tempo:      %E\n", dt);
    printf(" Coeficiente alpha:   %E\n", alpha);
    printf(LINHA);
    printf("ANÁLISE DE ESTABILIDADE:\n");
    printf(" Valor de r: %.4f (deve ser < 0.5 para estabilidade)\n", r);
    printf(" Status: %s\n", (r > 0.5) ? "INSTÁVEL!" : "Estável");
    printf(LINHA);

    double *u = malloc(sizeof(double) * n * n);
    double *u_novo = malloc(sizeof(double) * n * n);

    #pragma omp target enter data map(alloc: u[0:n*n], u_novo[0:n*n])

    initial_value(n, dx, comprimento, u);
    zero(n, u_novo);

    #pragma omp target update to(u[0:n*n], u_novo[0:n*n])

    printf("FASE DE COMPUTAÇÃO:\n");
    printf(" Executando em %s\n", omp_get_num_devices() > 0 ? "GPU" : "CPU");

    double tic = omp_get_wtime();
    for (int passo = 0; passo < npassos; ++passo) {
        solve(n, alpha, dx, dt, u, u_novo);

        double *temp = u;
        u = u_novo;
        u_novo = temp;

        if ((passo + 1) % (npassos / 10) == 0 || npassos < 10) {
            printf(" Completo passo %d/%d\n", passo + 1, npassos);
        }
    }
    
    #pragma omp target update from(u[0:n*n])
    
    double toc = omp_get_wtime();

    double erro = l2norm(n, u, npassos, dt, alpha, dx, comprimento);
    double fim = omp_get_wtime();

    printf(LINHA);
    printf("RESULTADOS DE DESEMPENHO:\n");
    printf(" Erro numérico (L2norm): %E\n", erro);
    printf(" Tempo de solução:       %.4f segundos\n", toc - tic);
    printf(" Tempo médio por passo:  %.4f segundos\n", (toc - tic) / npassos);
    printf(" Tempo total:            %.4f segundos\n", fim - inicio);
    if (toc - tic > 0.0) {
        printf(" Throughput:             %.2f milhões de células/segundo\n", 
               (n * n * npassos) / ((toc - tic) * 1e6));
    } else {
        printf(" Throughput:             N/A (tempo de solução muito pequeno)\n");
    }
    printf(LINHA);

    #pragma omp target exit data map(delete: u[0:n*n], u_novo[0:n*n])
    
    free(u);
    free(u_novo);
    return 0;
}


/*
#!/bin/bash
#SBATCH --partition=gpu-8-v100
#SBATCH --gpus-per-node=1
#SBATCH --nodes=1
#SBATCH --time=00:20:00
#SBATCH --job-name=020_heat_gpu
#SBATCH --output=020_heat_%j.out
#SBATCH --error=020_heat_%j.err

module load compilers/nvidia/nvhpc/24.11

nvc 020_tarefa.c -mp=gpu -o 020_tarefa -lm -O3

if [ $? -ne 0 ]; then
    echo "Falha na compilação!"
    exit 1
fi

SIZES=(1000 2000 4000 8000 16000)
NSTEPS=10

for SIZE in "${SIZES[@]}"; do
    echo "Executando para tamanho: ${SIZE}x${SIZE}"
    ./020_tarefa $SIZE $NSTEPS
    
    if [ $SIZE -le 8000 ]; then
        echo "Perfilamento para tamanho: ${SIZE}x${SIZE}"
        nsys profile --stats=true ./020_tarefa $SIZE $NSTEPS
    fi
done

*/