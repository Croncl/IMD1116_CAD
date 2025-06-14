#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define MAX_M 10000
#define MAX_N 10000

// Matrizes estáticas
double A[MAX_M][MAX_N];
double x[MAX_N];
double y[MAX_M];
double local_A[MAX_M][MAX_N];  // Parte local da matriz (colunas dispersas)
double local_x[MAX_N];         // Parte local do vetor x
double partial_y[MAX_M];       // Contribuição parcial para y

void print_table_row(int M, int N, int size, double time, double baseline_time) {
    double speedup = baseline_time / time;
    double efficiency = speedup / size * 100;
    printf("| %dx%-10d | %-13d | %-13.6f | %-13.2f | %-12.1f%% |\n", 
           M, N, size, time, speedup, efficiency);
}

void write_csv(const char* filename, int M, int N, int size, double time) {
    FILE *file = fopen(filename, "a");
    if (file != NULL) {
        fprintf(file, "%d,%d,%d,%.6f\n", M, N, size, time);
        fclose(file);
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    int M, N;
    int *counts, *displs;
    double start_time, end_time;
    MPI_Datatype col_type, resized_col_type;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Leitura de argumentos
    if (argc != 3) {
        if (rank == 0) {
            printf("Uso: %s <linhas M> <colunas N>\n", argv[0]);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    M = atoi(argv[1]);
    N = atoi(argv[2]);

    if (M > MAX_M || N > MAX_N) {
        if (rank == 0) {
            printf("Erro: Dimensões máximas excedidas\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Alocação dos vetores de contagem e deslocamento
    counts = (int*) malloc(size * sizeof(int));
    displs  = (int*) malloc(size * sizeof(int));
    int base = N / size;
    int resto = N % size;
    int offset = 0;
    for (int i = 0; i < size; i++) {
        counts[i] = base + (i < resto ? 1 : 0);
        displs[i] = offset;
        offset += counts[i];
    }

    int local_N = counts[rank];

    // Inicialização no processo 0
    if (rank == 0) {
        srand(time(NULL));
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++)
                A[i][j] = (double)rand() / RAND_MAX;

        for (int j = 0; j < N; j++)
            x[j] = (double)rand() / RAND_MAX;

        start_time = MPI_Wtime();
    }

    // Tipo derivado para uma coluna
    MPI_Type_vector(M, 1, N, MPI_DOUBLE, &col_type);
    MPI_Type_create_resized(col_type, 0, sizeof(double), &resized_col_type);
    MPI_Type_commit(&resized_col_type);
    // Scatterv das colunas
    MPI_Scatterv(&(A[0][0]), counts, displs, resized_col_type,
                 &(local_A[0][0]), local_N * M, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);
    // Scatterv do vetor x
    MPI_Scatterv(x, counts, displs, MPI_DOUBLE,
                 local_x, local_N, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);
    // Inicializa o vetor parcial
    for (int i = 0; i < M; i++) {
        partial_y[i] = 0.0;
        for (int j = 0; j < local_N; j++) {
            partial_y[i] += local_A[i][j] * local_x[j];
        }
    }
    // Redução das contribuições para y
    MPI_Reduce(partial_y, y, M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        end_time = MPI_Wtime();
        double elapsed = end_time - start_time;
        print_table_row(M, N, size, elapsed, elapsed);  // tempo base == atual
        write_csv("017_tarefa_mpi_results_cols.csv", M, N, size, elapsed);
    }

    MPI_Type_free(&col_type);
    MPI_Type_free(&resized_col_type);
    free(counts);
    free(displs);
    MPI_Finalize();
    return 0;
}
//mpicc 017_tarefa.c -o 017_tarefa
//mpiexec -n 2 ./017_tarefa 1000 1000
//mpiexec --oversubscribe -n 4 ./017_tarefa 1000 1000

/*
#!/bin/bash
#SBATCH --time=0-0:20
#SBATCH --partition=amd-512
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=1 
#SBATCH --job-name=017_tarefa_job
#SBATCH --output=017_tarefa_%j.out
#SBATCH --error=017_tarefa_%j.err

module load compilers/gnu/14.2.0

# Compila o código
mpicc 017_tarefa.c -o 017_tarefa -lm

# Defina os tamanhos das matrizes a serem testados
declare -a sizes=("1000 1000" "5000 5000" "10000 10000")

# Defina os números de processos a serem testados
declare -a procs=(1 2 4 8)


# Executa as combinações
for size in "${sizes[@]}"; do
    M=$(echo $size | cut -d' ' -f1)
    N=$(echo $size | cut -d' ' -f2)
    for p in "${procs[@]}"; do
        echo "Executando: $p processos | Matriz: ${M}x${N}"
        mpiexec -n $p ./017_tarefa $M $N
    done
done

copiar arquivos depois
scp -P 4422 clcronje@sc2.npad.ufrn.br:~/imd1116_cad/017_*.csv ~/Downloads/

*/