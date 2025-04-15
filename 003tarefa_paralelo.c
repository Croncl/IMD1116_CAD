//gcc -D_POSIX_C_SOURCE=199309L -o 003tarefa_paralelo 003tarefa_paralelo.c -lm

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Função para calcular PI com 'precisão' casas decimais usando BBP
void calcular_pi(int precisao) {
    int i;
    double pi = 0.0;

    // Ajuste do número de iterações para garantir a precisão
    int iteracoes = (int)(precisao * 3.321928); // log2(10) ≈ 3.321928 (conversão dígitos hex → dec)

    for (i = 0; i < iteracoes; i++) {
        pi += (1.0 / pow(16, i)) * (
              4.0 / (8 * i + 1) -
              2.0 / (8 * i + 4) -
              1.0 / (8 * i + 5) -
              1.0 / (8 * i + 6));
    }

    printf("π ≈ %.*f\n", precisao, pi);
}

int main() {
    int n;
    clock_t inicio, fim;
    double tempo;

    printf("Digite o número de casas decimais de π: ");
    scanf("%d", &n);

    inicio = clock();
    calcular_pi(n);
    fim = clock();

    tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC * 1e9; // Tempo em nanossegundos

    printf("Tempo de execução: %.0f ns\n", tempo);

    return 0;
}