#include <stdio.h>
#include <cuda_runtime.h>

#define NX 300
#define NY 300
#define NZ 300
#define NSTEPS 2000

const double nu = 0.1;
const double dt = 0.001;

__global__ void atualiza(double *u, double *u_new, int nx, int ny, int nz, double nu, double dt) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z * blockDim.z + threadIdx.z;

    // índice linear na ordem i*NY*NZ + j*NZ + k
    if (x >= 1 && x < nx-1 && y >= 1 && y < ny-1 && z >= 1 && z < nz-1) {
        int idx = x * ny * nz + y * nz + z;

        // índices dos vizinhos
        int idx_xm = (x-1) * ny * nz + y * nz + z;
        int idx_xp = (x+1) * ny * nz + y * nz + z;
        int idx_ym = x * ny * nz + (y-1) * nz + z;
        int idx_yp = x * ny * nz + (y+1) * nz + z;
        int idx_zm = x * ny * nz + y * nz + (z-1);
        int idx_zp = x * ny * nz + y * nz + (z+1);

        // cálculo da atualização pelo esquema explícito
        double laplaciano = (u[idx_xp] + u[idx_xm] + u[idx_yp] + u[idx_ym] + u[idx_zp] + u[idx_zm] - 6.0 * u[idx]) / (1.0 * 1.0);
        u_new[idx] = u[idx] + nu * dt * laplaciano;
    }
}

int main() {
    size_t size = NX * NY * NZ * sizeof(double);
    double *h_u = (double*)malloc(size);
    double *d_u, *d_u_new;

    // Inicializa condição inicial: 0.0 em todo lugar, exceto centro com 100.0
    for (int i=0; i<NX; i++) {
        for (int j=0; j<NY; j++) {
            for (int k=0; k<NZ; k++) {
                int idx = i * NY * NZ + j * NZ + k;
                h_u[idx] = 0.0;
            }
        }
    }
    int cx = NX / 2;
    int cy = NY / 2;
    int cz = NZ / 2;
    h_u[cx * NY * NZ + cy * NZ + cz] = 100.0;

    cudaMalloc(&d_u, size);
    cudaMalloc(&d_u_new, size);
    cudaMemcpy(d_u, h_u, size, cudaMemcpyHostToDevice);

    dim3 threads(8,8,8);
    dim3 blocks((NX + threads.x - 1)/threads.x,
                (NY + threads.y - 1)/threads.y,
                (NZ + threads.z - 1)/threads.z);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    for (int step = 0; step < NSTEPS; step++) {
        atualiza<<<blocks, threads>>>(d_u, d_u_new, NX, NY, NZ, nu, dt);
        cudaDeviceSynchronize();

        // troca ponteiros
        double *temp = d_u;
        d_u = d_u_new;
        d_u_new = temp;
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);

    // copia resultado final para host
    cudaMemcpy(h_u, d_u, size, cudaMemcpyDeviceToHost);

    // imprime valor central
    printf("Valor no centro após %d passos: %f\n", NSTEPS, h_u[cx * NY * NZ + cy * NZ + cz]);
    printf("Tempo total CUDA: %f ms\n", milliseconds);

    // libera memória
    cudaFree(d_u);
    cudaFree(d_u_new);
    free(h_u);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return 0;
}


/*
#!/bin/bash
#SBATCH --partition=gpu-4-a100        # Partição com GPUs V100/A100
#SBATCH --gpus-per-node=1             # 1 GPU por nó
#SBATCH --nodes=1                     # 1 nó
#SBATCH --time=00:10:00               # Tempo máximo (10 minutos)
#SBATCH --job-name=023_tarefa_cuda    # Nome do job
#SBATCH --output=023_tarefa_cuda_%j.out
#SBATCH --error=023_tarefa_cuda_%j.err

module load compilers/nvidia/cuda/12.6
module load compilers/gnu/11.2

# Compilar o código CUDA
nvcc 023_tarefa_cuda.cu -o 023_tarefa_cuda -lm

echo "Iniciando simulação CUDA 3D com perfilamento NSYS..."

# Executar a simulação com perfilamento NSYS, saída ficará no .out padrão
nsys profile --stats=true -o 023_tarefa_cuda_profile_${SLURM_JOB_ID} ./023_tarefa_cuda

echo "========================================"
echo "Simulação e perfilamento concluídos."
echo "- Perfil NSYS salvo em: 023_tarefa_cuda_profile_${SLURM_JOB_ID}.qdrep"
echo "Use Nsight Systems GUI para abrir e analisar o perfil."


*/

