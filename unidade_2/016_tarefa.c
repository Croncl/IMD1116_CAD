#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define MAX_M 10000
#define MAX_N 10000
#define REPEATS 1000  // Número de repetições para melhorar medição do tempo

// Matrizes estáticas
double A[MAX_M][MAX_N];
double x[MAX_N];
double y[MAX_M];
double local_A[MAX_M][MAX_N];  // Pode ser menor que MAX_M
double local_y[MAX_M];

void print_table_header() {
    printf("\n+---------------+---------------+---------------+---------------+---------------+\n");
    printf("| %-13s | %-13s | %-13s | %-13s | %-13s |\n", "Matriz (MxN)", "Processos", "Tempo (s)", "Speedup", "Eficiência");
    printf("+---------------+---------------+---------------+---------------+---------------+\n");
}

void print_table_row(int M, int N, int size, double time, double baseline_time) {
    double speedup = baseline_time / time;
    double efficiency = (speedup / size) * 100.0;
    printf("| %dx%-10d | %-13d | %-13.6f | %-13.2f | %-12.1f%% |\n", 
           M, N, size, time, speedup, efficiency);
}

void write_csv(const char* filename, int M, int N, int size, double time, double speedup, double efficiency, int is_first_write) {
    FILE *file = fopen(filename, is_first_write ? "w" : "a");  // "w" se for a primeira vez, "a" caso contrário
    if (file == NULL) {
        printf("Erro ao abrir arquivo CSV!\n");
        return;
    }
    if (is_first_write) {
        fprintf(file, "M,N,Processos,Tempo,Speedup,Eficiência\n");
    }
    fprintf(file, "%d,%d,%d,%.6f,%.2f,%.1f\n", M, N, size, time, speedup, efficiency);
    fclose(file);
}

// Função para salvar o tempo de referência (1 processo)
void save_baseline_time(double time) {
    FILE *file = fopen("baseline_time.txt", "w");
    if (file == NULL) return;
    fprintf(file, "%.6f", time);
    fclose(file);
}

// Função para carregar o tempo de referência
double load_baseline_time() {
    FILE *file = fopen("baseline_time.txt", "r");
    if (file == NULL) return -1.0;
    double time;
    fscanf(file, "%lf", &time);
    fclose(file);
    return time;
}

int main(int argc, char *argv[]) {
    int rank, size;
    int M, N;
    int local_M;
    int is_first_write = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Verificação de argumentos
    if (argc != 3) {
        if (rank == 0) printf("Uso: %s <linhas M> <colunas N>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    M = atoi(argv[1]);
    N = atoi(argv[2]);

    // Impede execução se M não for divisível pelo número de processos
    if (M % size != 0) {
        if (rank == 0) {
            printf("Erro: número de linhas M (%d) não é divisível pelo número de processos (%d).\n", M, size);
            printf("Cada processo precisa receber o mesmo número de linhas. Tente usar M múltiplo de %d.\n", size);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Verificação de dimensões máximas
    if (M > MAX_M || N > MAX_N) {
        if (rank == 0) {
            printf("Erro: Dimensões máximas excedidas (MAX_M = %d, MAX_N = %d)\n", MAX_M, MAX_N);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Verifica se o CSV ainda não existe para escrever cabeçalho (apenas no rank 0)
    if (rank == 0) {
        FILE *f = fopen("016_tarefa_mpi_results_static.csv", "r");
        if (f == NULL) {
            is_first_write = 1;  // Cabeçalho precisa ser escrito
        } else {
            fclose(f);
        }
    }

    // Cálculo de linhas por processo
    local_M = M / size;

    // Inicialização da matriz A e vetor x apenas no rank 0
    if (rank == 0) {
        srand(time(NULL));
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = (double)rand() / RAND_MAX;
            }
        }
        for (int j = 0; j < N; j++) {
            x[j] = (double)rand() / RAND_MAX;
        }
    }

    // Broadcast do vetor x para todos os processos
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Scatter das linhas da matriz A para cada processo
    MPI_Scatter(A, local_M * N, MPI_DOUBLE,
                local_A, local_M * N, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD); // Sincronização antes da medição

    double start_time = MPI_Wtime();

    // Cálculo local do produto y = A • x
    for (int r = 0; r < REPEATS; r++) {
        for (int i = 0; i < local_M; i++) {
            local_y[i] = 0.0;
            for (int j = 0; j < N; j++) {
                local_y[i] += local_A[i][j] * x[j];
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD); // Sincronização antes de finalizar a medição
    double end_time = MPI_Wtime();
    double elapsed_time = (end_time - start_time) / REPEATS;

    // Garante tempo mínimo positivo
    if (elapsed_time < 1e-9) elapsed_time = 1e-9;

    // Junta os resultados de y no processo 0
    MPI_Gather(local_y, local_M, MPI_DOUBLE,
               y, local_M, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    // Processamento final e escrita do CSV apenas no rank 0
    if (rank == 0) {
        double baseline_time = load_baseline_time();
        if (size == 1) {
            save_baseline_time(elapsed_time);
            baseline_time = elapsed_time;
        }
        if (baseline_time < 1e-9) baseline_time = 1e-9;

        double speedup = baseline_time / elapsed_time;
        double efficiency = (speedup / size) * 100.0;

        print_table_header();
        print_table_row(M, N, size, elapsed_time, baseline_time);
        write_csv("016_tarefa_mpi_results_static.csv", M, N, size, elapsed_time, speedup, efficiency, is_first_write);
    }

    MPI_Finalize();
    return 0;
}


//mpicc 016_tarefa.c -o 016_tarefa
//mpiexec -n 2 ./016_tarefa 1000 1000
//mpiexec --oversubscribe -n 4 ./016_tarefa 1000 1000

/*
!/bin/bash
#SBATCH --time=0-0:20
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1 
#SBATCH --job-name=016_tarefa_job
#SBATCH --output=016_tarefa_%j.out
#SBATCH --error=016_tarefa_%j.err

module load compilers/gnu/14.2.0

# Compila o código
mpicc 016_tarefa.c -o 016_tarefa -lm

# Defina os tamanhos das matrizes a serem testados
declare -a sizes=("500 500" "1000 1000" "2000 2000")

# Defina os números de processos a serem testados
declare -a procs=(1 2 4 8)

# Limpa o CSV e baseline se quiser começar do zero
rm -f 016_tarefa_mpi_results_static.csv baseline_time.txt


# Executa as combinações
for size in "${sizes[@]}"; do
    M=$(echo $size | cut -d' ' -f1)
    N=$(echo $size | cut -d' ' -f2)
    for p in "${procs[@]}"; do
        echo "Executando: $p processos | Matriz: ${M}x${N}"
        mpiexec --oversubscribe -n $p ./016_tarefa $M $N
    done
done

copiar arquivos depois
scp -P 4422 clcronje@sc2.npad.ufrn.br:~/imd1116_cad/016_*.csv ~/Downloads/
scp -P 4422 clcronje@sc2.npad.ufrn.br:~/imd1116_cad/baseline* ~/Downloads/

*/