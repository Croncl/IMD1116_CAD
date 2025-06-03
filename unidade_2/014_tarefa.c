#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_REPETITIONS 1000
#define MIN_MSG_SIZE 8
#define MAX_MSG_SIZE (1 << 20) // 1MB

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            printf("Este programa deve ser executado com exatamente 2 processos MPI.\n");
            printf("Uso correto: mpirun -np 2 %s\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Aloca e inicializa buffer de mensagem
    char* message = malloc(MAX_MSG_SIZE);
    memset(message, 'A', MAX_MSG_SIZE);

    if (rank == 0) {
        // Cabeçalho formatado
        printf("\n==================================================\n");
        printf("Benchmark de Comunicação MPI (Ping-Pong)\n");
        printf("==================================================\n");
        printf("%-15s %15s\n", "Tamanho(bytes)", "Tempo(μs)");
        printf("------------------------------------------\n");
    }

    // Loop principal de testes
    for (int msg_size = MIN_MSG_SIZE; msg_size <= MAX_MSG_SIZE; msg_size *= 2) {
        double total_time = 0.0;
        // Repete o teste várias vezes para obter média estável
        for (int rep = 0; rep < NUM_REPETITIONS; rep++) {
            MPI_Barrier(MPI_COMM_WORLD);
            double start = MPI_Wtime();  
            if (rank == 0) {
                // Processo 0 envia e recebe de volta
                MPI_Send(message, msg_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(message, msg_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                // Processo 1 recebe e envia de volta
                MPI_Recv(message, msg_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(message, msg_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }    
            total_time += (MPI_Wtime() - start);
        }
        // Apenas o rank 0 imprime os resultados
        if (rank == 0) {
            double avg_time = (total_time / NUM_REPETITIONS) * 1e6; // μs
            printf("%-15d %15.2f\n", msg_size, avg_time);
        }
    }

    if (rank == 0) {
        printf("------------------------------------------\n");
        printf("Teste concluído com sucesso.\n");
        printf("==================================================\n\n");
    }

    free(message);
    MPI_Finalize();
    return 0;
}
//mpicc 014_tarefa.c -o 014_tarefa -lm
//mpirun -np 2 ./014_tarefa_copy


/* mudas tarefa por no 
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1 

#!/bin/bash
#SBATCH --time=0-0:20
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1 
#SBATCH --job-name=014_tarefa_job
#SBATCH --output=014_tarefa_%j.out
#SBATCH --error=014_tarefa_%j.err

module load compilers/gnu/14.2.0

# Compila o código
mpicc 014_tarefa.c -o 014_tarefa -lm

# Executa a simulação
mpirun -np 2 ./014_tarefa_copy


*/