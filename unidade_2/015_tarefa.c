#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

// Parâmetros da simulação
#define TOTAL_CELLS 1000000  // Total de células na barra
#define TIME_STEPS 1000      // Número de iterações temporais
#define ALPHA 0.1           // Coeficiente de difusão térmica

// Estrutura para armazenar métricas de tempo
typedef struct {
    double total;
    double computation;
    double communication;
} TimingMetrics;

// Função para inicializar a temperatura na barra
void initialize(double *temperature, int local_cells, int rank) {
    for (int i = 0; i < local_cells; i++) {
        // Inicialização com gradiente linear mais perturbação aleatória
        temperature[i] = (rank + (double)i/local_cells) + 0.1*((double)rand()/RAND_MAX);
    }
}

// Função para atualizar a temperatura (cálculo principal)
void update_temperature(double *current, double *next, int local_cells) {
    for (int i = 1; i < local_cells-1; i++) {
        next[i] = current[i] + ALPHA * (current[i-1] - 2*current[i] + current[i+1]);
    }
}

// Versão 1: Comunicação bloqueante (MPI_Send/MPI_Recv)
void simulate_blocking(int rank, int size, int local_cells, TimingMetrics *metrics) {
    // Alocação com 2 células extras (fantasmas) para comunicação
    double *current = (double *)malloc((local_cells + 2) * sizeof(double));
    double *next = (double *)malloc((local_cells + 2) * sizeof(double));
    
    initialize(current + 1, local_cells, rank);  // +1 para deixar espaço para célula fantasma esquerda
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int t = 0; t < TIME_STEPS; t++) {
        // Comunicação com vizinhos
        double comm_start = MPI_Wtime();
        
        // Envia borda direita para o processo à direita (se existir)
        if (rank < size - 1) {
            MPI_Send(&current[local_cells], 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
        }
        
        // Recebe borda esquerda do processo à esquerda (se existir)
        if (rank > 0) {
            MPI_Recv(&current[0], 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // Envia borda esquerda para o processo à esquerda (se existir)
        if (rank > 0) {
            MPI_Send(&current[1], 1, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD);
        }
        
        // Recebe borda direita do processo à direita (se existir)
        if (rank < size - 1) {
            MPI_Recv(&current[local_cells + 1], 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        metrics->communication += MPI_Wtime() - comm_start;
        
        // Atualização da temperatura
        double comp_start = MPI_Wtime();
        update_temperature(current, next, local_cells + 2);
        metrics->computation += MPI_Wtime() - comp_start;
        
        // Troca os ponteiros para a próxima iteração
        double *temp = current;
        current = next;
        next = temp;
    }
    
    gettimeofday(&end, NULL);
    metrics->total = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    free(current);
    free(next);
}

// Versão 2: Comunicação não bloqueante com MPI_Wait
void simulate_nonblocking_wait(int rank, int size, int local_cells, TimingMetrics *metrics) {
    double *current = (double *)malloc((local_cells + 2) * sizeof(double));
    double *next = (double *)malloc((local_cells + 2) * sizeof(double));
    
    initialize(current + 1, local_cells, rank);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int t = 0; t < TIME_STEPS; t++) {
        MPI_Request requests[4];
        int request_count = 0;
        
        double comm_start = MPI_Wtime();
        
        // Comunicação não bloqueante
        if (rank < size - 1) {
            MPI_Isend(&current[local_cells], 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(&current[local_cells + 1], 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &requests[request_count++]);
        }
        
        if (rank > 0) {
            MPI_Isend(&current[1], 1, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(&current[0], 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
        }
        
        // Atualização dos pontos internos (que não dependem das células fantasmas)
        double comp_start = MPI_Wtime();
        for (int i = 2; i < local_cells; i++) {
            next[i] = current[i] + ALPHA * (current[i-1] - 2*current[i] + current[i+1]);
        }
        metrics->computation += MPI_Wtime() - comp_start;
        
        // Espera pela conclusão das comunicações
        MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);
        metrics->communication += MPI_Wtime() - comm_start;
        
        // Atualiza os pontos de borda que dependem das células fantasmas
        comp_start = MPI_Wtime();
        if (local_cells > 1) {
            next[1] = current[1] + ALPHA * (current[0] - 2*current[1] + current[2]);
            next[local_cells] = current[local_cells] + ALPHA * (current[local_cells-1] - 2*current[local_cells] + current[local_cells+1]);
        }
        metrics->computation += MPI_Wtime() - comp_start;
        //troca dos ponteiros
        double *temp = current;
        current = next;
        next = temp;
    }
    
    gettimeofday(&end, NULL);
    metrics->total = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    free(current);
    free(next);
}

// Versão 3: Comunicação não bloqueante com MPI_Test
void simulate_nonblocking_test(int rank, int size, int local_cells, TimingMetrics *metrics) {
    double *current = (double *)malloc((local_cells + 2) * sizeof(double));
    double *next = (double *)malloc((local_cells + 2) * sizeof(double));

    initialize(current + 1, local_cells, rank);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int t = 0; t < TIME_STEPS; t++) {
        MPI_Request requests[4];
        int request_count = 0;
        int completed = 0;
        
        double comm_start = MPI_Wtime();
        
        // Comunicação não bloqueante
        if (rank < size - 1) {
            MPI_Isend(&current[local_cells], 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(&current[local_cells + 1], 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &requests[request_count++]);
        }
        
        if (rank > 0) {
            MPI_Isend(&current[1], 1, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(&current[0], 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
        }
        
        // Atualização dos pontos internos enquanto verifica a comunicação
        double comp_start = MPI_Wtime();
        for (int i = 2; i < local_cells; i++) {
            next[i] = current[i] + ALPHA * (current[i-1] - 2*current[i] + current[i+1]);
            
            // Verifica periodicamente se a comunicação foi concluída
            if (i % 100 == 0 && completed < request_count) {
                for (int j = 0; j < request_count; j++) {
                    int flag;
                    MPI_Test(&requests[j], &flag, MPI_STATUS_IGNORE);
                    if (flag) completed++;
                }
            }
        }
        metrics->computation += MPI_Wtime() - comp_start;
        
        // Garante que toda comunicação foi concluída
        while (completed < request_count) {
            for (int j = 0; j < request_count; j++) {
                int flag;
                MPI_Test(&requests[j], &flag, MPI_STATUS_IGNORE);
                if (flag) completed++;
            }
        }
        metrics->communication += MPI_Wtime() - comm_start;
        
        // Atualiza os pontos de borda que dependem das células fantasmas
        comp_start = MPI_Wtime();
        if (local_cells > 1) {
            next[1] = current[1] + ALPHA * (current[0] - 2*current[1] + current[2]);
            next[local_cells] = current[local_cells] + ALPHA * (current[local_cells-1] - 2*current[local_cells] + current[local_cells+1]);
        }
        metrics->computation += MPI_Wtime() - comp_start;
        
        double *temp = current;
        current = next;
        next = temp;
    }
    
    gettimeofday(&end, NULL);
    metrics->total = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    free(current);
    free(next);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Dividir o domínio entre os processos
    int base_cells = TOTAL_CELLS / size;
    int remainder = TOTAL_CELLS % size;
    int local_cells = base_cells + (rank < remainder ? 1 : 0);
    
    TimingMetrics blocking = {0}, nonblocking_wait = {0}, nonblocking_test = {0};
    
    // Executar e medir as três versões
    if (rank == 0) printf("Executando simulação com comunicação bloqueante...\n");
    simulate_blocking(rank, size, local_cells, &blocking);
    
    if (rank == 0) printf("Executando simulação com comunicação não bloqueante (Wait)...\n");
    simulate_nonblocking_wait(rank, size, local_cells, &nonblocking_wait);
    
    if (rank == 0) printf("Executando simulação com comunicação não bloqueante (Test)...\n");
    simulate_nonblocking_test(rank, size, local_cells, &nonblocking_test);
    
    // Coletar e exibir resultados de tempo
    if (rank == 0) {
        printf("\nResultados (tempos em segundos):\n");
        printf("-------------------------------------------------\n");
        printf("Versão               | Total   | Computação | Comunicação\n");
        printf("-------------------------------------------------\n");
        printf("Bloqueante           | %-7.3f | %-10.3f | %-10.3f\n", 
               blocking.total, blocking.computation, blocking.communication);
        printf("Não bloqueante (Wait)| %-7.3f | %-10.3f | %-10.3f\n", 
               nonblocking_wait.total, nonblocking_wait.computation, nonblocking_wait.communication);
        printf("Não bloqueante (Test)| %-7.3f | %-10.3f | %-10.3f\n", 
               nonblocking_test.total, nonblocking_test.computation, nonblocking_test.communication);
        printf("-------------------------------------------------\n");
        
        // Calcular e mostrar ganhos
        double gain_wait = (blocking.total - nonblocking_wait.total) / blocking.total * 100;
        double gain_test = (blocking.total - nonblocking_test.total) / blocking.total * 100;
        
        printf("\nGanhos de desempenho:\n");
        printf("Não bloqueante (Wait): %.2f%% mais rápido\n", gain_wait);
        printf("Não bloqueante (Test): %.2f%% mais rápido\n", gain_test);
    }
    
    MPI_Finalize();
    return 0;
}
/*
mpicc 015_tarefa.c -o 015_tarefa -lm
mpirun -np 4 ./015_tarefa
*/