#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 1000

int main() {
    float a[N], b[N], c[N];
    
    printf("Iniciando programa de soma de vetores com OpenMP offloading...\n\n");
    
    // Inicialização dos vetores na CPU
    printf("Inicializando vetores na CPU...\n");
    for(int i = 0; i < N; i++) {
        a[i] = i * 1.0f;
        b[i] = i * 2.0f;
    }
    
    printf("Primeiros 5 elementos antes do processamento:\n");
    printf("a[0-4]: ");
    for(int i = 0; i < 5; i++) printf("%.1f ", a[i]);
    printf("\nb[0-4]: ");
    for(int i = 0; i < 5; i++) printf("%.1f ", b[i]);
    printf("\n\n");

    // Região de computação na GPU
    printf("Transferindo dados para GPU e executando calculos...\n");
    double start_time = omp_get_wtime();
    
    #pragma omp target map(to: a, b) map(from: c)
    {
        printf("[GPU] Numero de dispositivos disponiveis: %d\n", omp_get_num_devices());
        printf("[GPU] Executando em dispositivo %d\n", omp_get_device_num());
        
        #pragma omp parallel for
        for(int i = 0; i < N; i++) {
            c[i] = a[i] + b[i];
        }
    }
    
    double end_time = omp_get_wtime();
    printf("Calculo concluido em %.6f segundos\n\n", end_time - start_time);

    // Verificação dos resultados na CPU
    printf("Resultados (10 primeiros elementos):\n");
    printf("Indice |   a   |   b   |   c (a+b)\n");
    printf("----------------------------------\n");
    for(int i = 0; i < 10; i++) {
        printf("%5d | %5.1f | %5.1f | %5.1f\n", i, a[i], b[i], c[i]);
    }

    // Verificação de um elemento no meio do vetor
    printf("\nVerificacao do elemento no meio do vetor:\n");
    printf("c[%d] = %.1f (esperado: %.1f)\n", N/2, c[N/2], a[N/2] + b[N/2]);

    return 0;
}
/*

gcc -fopenmp -o exemplo_target exemplo_target.c

OMP_TARGET_OFFLOAD=MANDATORY ./exemplo_target

*/