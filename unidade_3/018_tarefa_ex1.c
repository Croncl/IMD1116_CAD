#include <stdio.h>
#include <omp.h>

#define N 1000          // Tamanho dos vetores
#define TOL 1e-6        // Tolerância para comparação de valores float

int main() {
    float a[N], b[N], c[N], res[N];  // Declaração dos vetores
    int err = 0;                     // Contador de erros

    // 1. Inicialização dos vetores (paralelizada)
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        a[i] = (float)i;           // Preenche a com valores 0, 1, 2, ..., N-1
        b[i] = 2.0f * (float)i;    // Preenche b com valores 0, 2, 4, ..., 2(N-1)
        c[i] = 0.0f;               // Inicializa c com zeros
        res[i] = i + 2 * i;        // Calcula o resultado esperado (a[i] + b[i])
    }

    // 2. Operação de soma vetorial (paralelizada)
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        c[i] = a[i] + b[i];       // Soma os elementos de a e b em c
    }

    // 3. Verificação dos resultados (paralelizada com redução)
    #pragma omp parallel for reduction(+:err)
    for (int i = 0; i < N; i++) {
        float val = c[i] - res[i]; // Diferença entre valor calculado e esperado
        val = val * val;           // Quadrado da diferença (para eliminar sinais)
        if (val > TOL) err++;      // Incrementa contador se diferença > tolerância
    }

    printf("Vetores somados com %d erros\n", err);
    return 0;
}