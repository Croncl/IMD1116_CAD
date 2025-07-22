#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>
#include <string.h>

// Parâmetros da simulação
#define PLATE_SIZE_SMALL 256
#define PLATE_SIZE_MEDIUM 512
#define PLATE_SIZE_LARGE 1024
#define TIME_STEPS 10000
#define ALPHA 0.1

typedef struct {
    double total;
    double computation;
    double communication;
} TimingMetrics;

void initialize(double **plate, int rows, int cols, int global_start_row) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double x = (double)(global_start_row + i) / (rows * omp_get_num_procs());
            double y = (double)j / cols;
            plate[i][j] = x + y + 0.1 * ((double)rand() / RAND_MAX);
        }
    }
}

void update_temperature(double **current, double **next, int start_row, int end_row, int cols) {
    #pragma omp parallel for collapse(2)
    for (int i = start_row; i < end_row; i++) {
        for (int j = 1; j < cols-1; j++) {
            next[i][j] = current[i][j] + ALPHA * (
                current[i-1][j] + current[i+1][j] + 
                current[i][j-1] + current[i][j+1] - 
                4 * current[i][j]
            );
        }
    }
}

void simulate_hybrid(int rank, int size, int plate_size, TimingMetrics *metrics) {
    int base_rows = plate_size / size;
    int remainder = plate_size % size;
    int local_rows = base_rows + (rank < remainder ? 1 : 0);
    int global_start_row = rank * base_rows + (rank < remainder ? rank : remainder);
    
    double **current = malloc((local_rows + 2) * sizeof(double *));
    double **next = malloc((local_rows + 2) * sizeof(double *));
    for (int i = 0; i < local_rows + 2; i++) {
        current[i] = malloc(plate_size * sizeof(double));
        next[i] = malloc(plate_size * sizeof(double));
    }

    initialize(current + 1, local_rows, plate_size, global_start_row);
    
    double total_start = MPI_Wtime();
    double comp_time = 0.0;
    double comm_time = 0.0;

    if (size == 1) {
        for (int t = 0; t < TIME_STEPS; t++) {
            double comp_start = MPI_Wtime();
            update_temperature(current, next, 1, local_rows + 1, plate_size);
            comp_time += MPI_Wtime() - comp_start;

            double **temp = current;
            current = next;
            next = temp;
        }
    } else {
        for (int t = 0; t < TIME_STEPS; t++) {
            MPI_Request requests[4];
            int req_count = 0;

            // Computa parte interna
            double comp_start = MPI_Wtime();
            update_temperature(current, next, 2, local_rows, plate_size);
            comp_time += MPI_Wtime() - comp_start;

            // Início da comunicação não bloqueante
            double comm_start = MPI_Wtime();

            if (rank > 0)
                MPI_Isend(current[1], plate_size, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
            if (rank < size - 1)
                MPI_Isend(current[local_rows], plate_size, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &requests[req_count++]);
            if (rank > 0)
                MPI_Irecv(current[0], plate_size, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &requests[req_count++]);
            if (rank < size - 1)
                MPI_Irecv(current[local_rows + 1], plate_size, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &requests[req_count++]);

            comm_time += MPI_Wtime() - comm_start;

            // Computa bordas enquanto comunicação ocorre
            comp_start = MPI_Wtime();
            if (rank > 0)
                update_temperature(current, next, 1, 2, plate_size);
            if (rank < size - 1)
                update_temperature(current, next, local_rows, local_rows + 1, plate_size);
            comp_time += MPI_Wtime() - comp_start;

            if (req_count > 0)
                MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);

            double **temp = current;
            current = next;
            next = temp;
        }
    }

    metrics->total = MPI_Wtime() - total_start;
    metrics->computation = comp_time;
    metrics->communication = comm_time;

    for (int i = 0; i < local_rows + 2; i++) {
        free(current[i]);
        free(next[i]);
    }
    free(current);
    free(next);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) fprintf(stderr, "Uso: %s [small|medium|large] [nós]\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int plate_size;
    if (strcmp(argv[1], "small") == 0) plate_size = PLATE_SIZE_SMALL;
    else if (strcmp(argv[1], "medium") == 0) plate_size = PLATE_SIZE_MEDIUM;
    else if (strcmp(argv[1], "large") == 0) plate_size = PLATE_SIZE_LARGE;
    else {
        if (rank == 0) fprintf(stderr, "Tamanho inválido: %s\n", argv[1]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int nodes = atoi(argv[2]);
    int threads_per_proc = (nodes > 0) ? (2 * size / nodes) : 2;
    omp_set_num_threads(threads_per_proc);

    TimingMetrics metrics = {0};
    simulate_hybrid(rank, size, plate_size, &metrics);

    // Coletar tempos máximos
    double total_time, comp_time, comm_time;
    MPI_Reduce(&metrics.total, &total_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&metrics.computation, &comp_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&metrics.communication, &comm_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        FILE *csv_file = fopen("resultados_simulacao.csv", "a");
        if (csv_file) {
            if (ftell(csv_file) == 0) {
                fprintf(csv_file, "problem_size,nodes,processes,threads_per_process,total_time,computation_time,communication_time\n");
            }
            fprintf(csv_file, "%s,%d,%d,%d,%.6f,%.6f,%.6f\n", 
                    argv[1], nodes, size, threads_per_proc, 
                    total_time, comp_time, comm_time);
            fclose(csv_file);
        }
    }

    MPI_Finalize();
    return 0;
}
