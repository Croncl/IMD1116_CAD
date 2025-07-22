#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cuda_runtime.h>

#define NX 300
#define NY 300
#define NZ 300
#define NSTEPS 2000

const double nu = 0.1;
const double dt = 0.01;
const double dx = 1.0;
const double alpha = nu * dt / (dx * dx);

__global__ void atualiza(double* vnew, double* vold, int nx, int ny, int nz, double alpha) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z * blockDim.z + threadIdx.z;
    
    if (x > 0 && x < nx-1 && y > 0 && y < ny-1 && z > 0 && z < nz-1) {
        int idx = z * ny * nx + y * nx + x;
        double laplacian = (vold[idx+1] + vold[idx-1] +
                          vold[idx+nx] + vold[idx-nx] +
                          vold[idx+nx*ny] + vold[idx-nx*ny] - 
                          6.0 * vold[idx]) / (dx * dx);
        vnew[idx] = vold[idx] + dt * nu * laplacian;
    }
}

void simulate_cuda(double *h_u, const char *filename) {
    double *d_u, *d_u_new;
    size_t size = NX * NY * NZ * sizeof(double);
    
    cudaMalloc(&d_u, size);
    cudaMalloc(&d_u_new, size);
    cudaMemcpy(d_u, h_u, size, cudaMemcpyHostToDevice);

    FILE *fp = fopen(filename, "w");
    fprintf(fp, "Passo,ValorCentral,Tempo\n");

    int bx = 8, by = 8, bz = 8;
    dim3 threads(bx, by, bz);
    dim3 grid((NX+bx-1)/bx, (NY+by-1)/by, (NZ+bz-1)/bz);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    for (int n = 0; n < NSTEPS; ++n) {
        cudaEventRecord(start);
        atualiza<<<grid, threads>>>(d_u_new, d_u, NX, NY, NZ, alpha);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);

        float ms;
        cudaEventElapsedTime(&ms, start, stop);
        double elapsed_step = ms / 1000.0;

        double *tmp = d_u;
        d_u = d_u_new;
        d_u_new = tmp;

        // Get center value
        double center_value;
        int center_idx = (NX/2) * NY * NZ + (NY/2) * NZ + (NZ/2);
        cudaMemcpy(&center_value, &d_u[center_idx], sizeof(double), cudaMemcpyDeviceToHost);
        
        fprintf(fp, "%d,%.5f,%.9f\n", n, center_value, elapsed_step);

        if (n % 200 == 0) {
            printf("[CUDA] Passo %d | Valor central: %.5f | Tempo passo: %.6f s\n",
                  n, center_value, elapsed_step);
        }
    }

    cudaMemcpy(h_u, d_u, size, cudaMemcpyDeviceToHost);
    fclose(fp);
    cudaFree(d_u);
    cudaFree(d_u_new);
}

int main() {
    printf("Iniciando simulação CUDA 3D\n");
    
    int size = NX * NY * NZ;
    double *h_u = (double*)malloc(size * sizeof(double));
    
    // Initialize with central peak
    for (int i = 0; i < NX; ++i)
        for (int j = 0; j < NY; ++j)
            for (int k = 0; k < NZ; ++k) {
                if ((i > NX/2-2 && i < NX/2+2) &&
                    (j > NY/2-2 && j < NY/2+2) &&
                    (k > NZ/2-2 && k < NZ/2+2))
                    h_u[i*NY*NZ + j*NZ + k] = 1.0;
                else
                    h_u[i*NY*NZ + j*NZ + k] = 0.0;
            }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    simulate_cuda(h_u, "simulacao_cuda.csv");
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                   (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Tempo total CUDA: %.3f segundos\n", elapsed);
    free(h_u);
    return 0;
}

/*
#!/bin/bash
#SBATCH --partition=gpu-8-v100
#SBATCH --gpus-per-node=1
#SBATCH --nodes=1
#SBATCH --time=00:10:00
#SBATCH --job-name=022_tarefa_cuda
#SBATCH --output=022_tarefa_cuda_%j.out
#SBATCH --error=022_tarefa_cuda_%j.err

module load compilers/nvidia/cuda/12.6
module load compilers/gnu/14.2.0

# Compilar
nvcc 022_tarefa_cuda.cu -o 022_tarefa_cuda -lm

# Executar e mover resultados
./022_tarefa_cuda
mv simulacao_cuda.csv "simulacao_cuda_${SLURM_JOB_ID}.csv"

echo "Simulação concluída. Resultados em simulacao_cuda_${SLURM_JOB_ID}.csv"

*/