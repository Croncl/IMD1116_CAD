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
