#include <stdio.h>
#include <omp.h>
#define N 10000000
#define TOL  0.0000001

int main()
{

    float a[N], b[N], c[N], res[N];
    int err=0;

    double init_time, compute_time, test_time;
    init_time    = -omp_get_wtime();

   // fill the arrays
    for (int i=0; i<N; i++){
        a[i] = (float)i;
        b[i] = 2.0*(float)i;
        c[i] = 0.0;
        res[i] = i + 2*i;
    }

    init_time    +=  omp_get_wtime();
    compute_time  = -omp_get_wtime();
   
   // add two vectors
    for (int i=0; i<N; i++){
        c[i] = a[i] + b[i];
    }

    compute_time +=  omp_get_wtime();
    test_time     = -omp_get_wtime();

   // test results
    for(int i=0;i<N;i++){
        float val = c[i] - res[i];
        val = val*val;
        if(val>TOL) err++;
    }

    test_time    +=  omp_get_wtime();
    printf(" vectors added with %d errors\n",err);

    printf("Init time:    %.3fs\n", init_time);
    printf("Compute time: %.3fs\n", compute_time);
    printf("Test time:    %.3fs\n", test_time);
    printf("Total time:   %.3fs\n", init_time + compute_time + test_time);
    return 0;
}
/*
#!/bin/bash
#SBATCH --partition=gpu-4-a100        # Usa uma GPU A100
#SBATCH --gpus-per-node=1
#SBATCH --nodes=1
#SBATCH --time=00:05:00
#SBATCH --job-name=018_tarefa
#SBATCH --output=018_tarefa-%j.out

ulimit -s unlimited
module load compilers/nvidia/nvhpc/24.11

# Compila com offloading para GPU NVIDIA
nvc 018_tarefa.c -mp=gpu -o 018_tarefa

# Executa com perfilador Nsight Systems
nsys profile -o 018_tarefa_report ./018_tarefa



*/