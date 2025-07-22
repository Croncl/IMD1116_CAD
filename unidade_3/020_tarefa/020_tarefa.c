#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

#define PI acos(-1.0)
#define LINHA "-----------------------------------------------------------------\n"

// Function declarations
void initial_value(const int n, const double dx, const double length, double * restrict u);
void zero(const int n, double * restrict u);
void solve(const int n, const double alpha, const double dx, const double dt, const double * restrict u, double * restrict u_tmp);
double solution(const double t, const double x, const double y, const double alpha, const double length);
double l2norm(const int n, const double * restrict u, const int nsteps, const double dt, const double alpha, const double dx, const double length);

int main(int argc, char *argv[]) {
    printf("\n=== SOLUÇÃO DA EQUAÇÃO DO CALOR ===\n");
    printf("=== Paralelização em GPU com OpenMP ===\n\n");
    printf("Objetivos da tarefa:\n");
    printf("1. Paralelizar o cálculo do estêncil térmico na GPU\n");
    printf("2. Analisar transferências de dados entre CPU e GPU\n");
    printf("3. Avaliar desempenho com diferentes tamanhos de problema\n\n");

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

    // Alocar dados na GPU uma vez no início
    #pragma omp target enter data map(alloc: u[0:n*n], u_novo[0:n*n])

    initial_value(n, dx, comprimento, u);
    zero(n, u_novo);

    // Transferir dados iniciais para GPU
    #pragma omp target update to(u[0:n*n], u_novo[0:n*n])

    printf("FASE DE COMPUTAÇÃO:\n");
    printf(" Executando em %s\n", omp_get_num_devices() > 0 ? "GPU" : "CPU");

    double tic = omp_get_wtime();
    for (int passo = 0; passo < npassos; ++passo) {
        solve(n, alpha, dx, dt, u, u_novo);

        // Troca de ponteiros
        double *temp = u;
        u = u_novo;
        u_novo = temp;

        if ((passo + 1) % (npassos / 10) == 0 || npassos < 10) {
            printf(" Completo passo %d/%d\n", passo + 1, npassos);
        }
    }
    
    // Transferir resultado final de volta para CPU
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
    if (toc - tic > 0.0)
        printf(" Throughput:             %.2f milhões de células/segundo\n", (n * n * npassos) / ((toc - tic) * 1e6));
    else
        printf(" Throughput:             N/A (tempo de solução muito pequeno)\n");
    printf(LINHA);
    printf(" Comando sugerido para profiling:\n");
    printf(" nsys profile --stats=true ./020_heat_optimized %d %d\n", n, npassos);
    printf(LINHA);

    // Liberar memória da GPU
    #pragma omp target exit data map(delete: u[0:n*n], u_novo[0:n*n])
    
    free(u);
    free(u_novo);
    return 0;
}

void initial_value(const int n, const double dx, const double length, double * restrict u) {
    double y = dx;
    #pragma omp parallel for collapse(2) if(n > 1000)
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            u[i + j * n] = sin(PI * x / length) * sin(PI * y / length);
            x += dx;
        }
        y += dx;
    }
}

void zero(const int n, double * restrict u) {
    #pragma omp parallel for collapse(2) if(n > 1000)
    for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i)
            u[i + j * n] = 0.0;
}

void solve(const int n, const double alpha, const double dx, const double dt, const double * restrict u, double * restrict u_tmp) {
    const double r = alpha * dt / (dx * dx);
    const double r2 = 1.0 - 4.0 * r;

    #pragma omp target teams distribute parallel for collapse(2) \
        map(to: u[0:n*n]) map(from: u_tmp[0:n*n]) nowait
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

double solution(const double t, const double x, const double y, const double alpha, const double length) {
    return exp(-2.0 * alpha * PI * PI * t / (length * length)) *
           sin(PI * x / length) * sin(PI * y / length);
}

double l2norm(const int n, const double * restrict u, const int nsteps, const double dt, const double alpha, const double dx, const double length) {
    double time = dt * (double)nsteps;
    double norm = 0.0;
    double y = dx;

    #pragma omp parallel for reduction(+:norm) collapse(2) if(n > 1000)
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


/*
#!/bin/bash
#SBATCH --partition=gpu-8-v100        # Partição com GPUs V100
#SBATCH --gpus-per-node=1             # 1 GPU por nó
#SBATCH --nodes=1                     # 1 nó
#SBATCH --time=00:10:00               # Aumentado para 10 minutos para 16000
#SBATCH --job-name=020_heat_gpu       # Nome do job atualizado
#SBATCH --output=020_heat_%j.out      # Arquivo de saída atualizado
#SBATCH --error=020_heat_%j.err       # Arquivo de erros atualizado

# Carrega módulos necessários
module load compilers/nvidia/nvhpc/24.11

# Compilação com suporte a GPU para versão otimizada
nvc 020_tarefa.c -mp=gpu -o 020_tarefa -lm

# Arquivo consolidado de resultados (nome atualizado)
RESULT_FILE="020_heat_results_${SLURM_JOB_ID}.csv"
echo "Tamanho,Passos,Tempo_Solucao,Throughput,Erro_L2" > $RESULT_FILE

# Define tamanhos de problema para teste (adicionado 16000)
SIZES=(1000 2000 4000 8000 16000)
NSTEPS=10

# Loop para executar com diferentes tamanhos
for SIZE in "${SIZES[@]}"; do
  echo "========================================"
  echo "Executando para grade ${SIZE}x${SIZE}..."
  
  # Ajusta tempo estimado baseado no tamanho
  if [ $SIZE -ge 16000 ]; then
    echo "  (Execução de grande porte - pode levar mais tempo)"
  fi
  
  # Execução normal (para resultados numéricos)
  ./020_tarefa $SIZE $NSTEPS > 020_heat_${SIZE}.log
  awk -v size=$SIZE -v steps=$NSTEPS '
    /Tempo de solu..o:/ {time=$4}
    /Throughput:/ {throughput=$3}
    /Erro num.rico/{error=$4}
    END {print size "," steps "," time "," throughput "," error}
  ' 020_heat_${SIZE}.log >> $RESULT_FILE
  
  # Perfilamento com nsys apenas para tamanhos ≤ 8000 (16000 pode ser muito grande)
  if [ $SIZE -le 8000 ]; then
    nsys profile -o 020_heat_profile_${SIZE} --stats=true ./020_tarefa $SIZE $NSTEPS
  else
    echo "  Perfilamento com nsys omitido para tamanho 16000 por requisitos de memória"
  fi
done

echo "========================================"
echo "Simulações completas. Resultados salvos em:"
echo "- Arquivo consolidado: $RESULT_FILE"
echo "- Logs individuais: 020_heat_[TAMANHO].log"
echo "- Dados de perfilamento: 020_heat_profile_[TAMANHO].nsys-rep (exceto 16000)"

*/