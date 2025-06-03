#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

// Função para alocar matriz MxN
double **allocate_matrix(int M, int N) {
    double **matrix = (double **)malloc(M * sizeof(double *));
    for (int i = 0; i < M; i++) {
        matrix[i] = (double *)malloc(N * sizeof(double));
    }
    return matrix;
}

// Função para liberar matriz
void free_matrix(double **matrix, int M) {
    for (int i = 0; i < M; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Função para inicializar matriz e vetor com valores aleatórios
void initialize(double **A, double *x, int M, int N) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (double)rand() / RAND_MAX;
        }
    }
    
    for (int j = 0; j < N; j++) {
        x[j] = (double)rand() / RAND_MAX;
    }
}

// Função para multiplicação local de parte da matriz pelo vetor
void local_matrix_vector_mult(double **local_A, double *x, double *local_y, int local_M, int N) {
    for (int i = 0; i < local_M; i++) {
        local_y[i] = 0.0;
        for (int j = 0; j < N; j++) {
            local_y[i] += local_A[i][j] * x[j];
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    int M, N;
    double **A, *x, *y;
    double **local_A, *local_y;
    int local_M;
    double start_time, end_time;
    FILE *csv_file;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Verifica argumentos
    if (argc != 3) {
        if (rank == 0) {
            printf("Uso: %s <linhas M> <colunas N>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    M = atoi(argv[1]);
    N = atoi(argv[2]);
    
    // Calcula número de linhas por processo
    local_M = M / size;
    if (rank < M % size) {
        local_M++;
    }
    
    // Aloca e inicializa matriz e vetor no processo 0
    if (rank == 0) {
        A = allocate_matrix(M, N);
        x = (double *)malloc(N * sizeof(double));
        y = (double *)malloc(M * sizeof(double));
        
        srand(time(NULL));
        initialize(A, x, M, N);
        
        // Abre arquivo CSV para escrita (append)
        csv_file = fopen("timings.csv", "a");
        if (csv_file == NULL) {
            printf("Erro ao abrir arquivo CSV.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        start_time = MPI_Wtime();  // Inicia contagem do tempo
    }
    
    // Aloca espaço para a parte local da matriz e do vetor y
    local_A = allocate_matrix(local_M, N);
    local_y = (double *)malloc(local_M * sizeof(double));
    
    // Distribui as linhas da matriz usando MPI_Scatter
    // Primeiro enviamos quantas linhas cada processo receberá
    int *sendcounts = NULL;
    int *displs = NULL;
    
    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        
        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = (M / size + (i < M % size)) * N;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }
    
    // Scatter das linhas da matriz
    MPI_Scatterv(rank == 0 ? &A[0][0] : NULL, sendcounts, displs, MPI_DOUBLE,
                 &local_A[0][0], local_M * N, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);
    
    // Broadcast do vetor x para todos os processos
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Multiplicação local
    local_matrix_vector_mult(local_A, x, local_y, local_M, N);
    
    // Coleta os resultados usando MPI_Gather
    // Prepara arrays para gatherv (se necessário)
    int *recvcounts = NULL;
    int *recvdispls = NULL;
    
    if (rank == 0) {
        recvcounts = (int *)malloc(size * sizeof(int));
        recvdispls = (int *)malloc(size * sizeof(int));
        
        int offset = 0;
        for (int i = 0; i < size; i++) {
            recvcounts[i] = M / size + (i < M % size);
            recvdispls[i] = offset;
            offset += recvcounts[i];
        }
    }
    
    MPI_Gatherv(local_y, local_M, MPI_DOUBLE,
                y, recvcounts, recvdispls, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // Finaliza contagem do tempo e exibe resultados
    if (rank == 0) {
        end_time = MPI_Wtime();
        double elapsed_time = end_time - start_time;
        
        // Exibe tabela de resultados
        printf("\n+-----------------+-----------------+-----------------+-----------------+\n");
        printf("| %-15s | %-15s | %-15s | %-15s |\n", "Matriz MxN", "Processos", "Tempo (s)", "Speedup");
        printf("+-----------------+-----------------+-----------------+-----------------+\n");
        printf("| %-15s | %-15d | %-15.6f | %-15s |\n", 
               argv[1] + "x" + argv[2], size, elapsed_time, "1.00");
        
        // Escreve no arquivo CSV
        fprintf(csv_file, "%d,%d,%d,%.6f\n", M, N, size, elapsed_time);
        fclose(csv_file);
        
        // Limpeza
        free_matrix(A, M);
        free(x);
        free(y);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(recvdispls);
    }
    
    // Limpeza local
    free_matrix(local_A, local_M);
    free(local_y);
    
    MPI_Finalize();
    return 0;
}
//mpicc 016_tarefa.c -o 016_tarefa
//mpiexec -n 4 ./016_tarefa 1000 1000
