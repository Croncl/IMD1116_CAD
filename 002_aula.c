/*tarefa 2 implementar tres lacos em c para verificar os efeitos do paralelismo a nivel de instrucao(ILP). 1)inicialize um vetor com um calculo simples.2) some seus elementos de forma cumulativa.3) quebre essa dependencia utilizando multiplas variaveis. Compare o tempo das versoes compiladas com diferentes niveis de otimizacao(00, 02, 03), e analise como o estilo de codigo e as dependencias influenciam o desempenho.

Paralelismo a nivel de instrucao:
Pipelining
Vetorizacao

sum1 = sum0 + a[1]

Para compilar:
compilar: gcc -o nomequeescolhio0 -o0  - compiar com zero otimizacao do compilador

gcc -o nomequeescolhio2 -o2  compilador vai ser mais cuidadoso

gcc -o nomequeescolhio3 -o3 compilador nao garante que vai ser correto sempre


*/

#include <stdio.h>
#include <stdlib.h>
//essa soma e recurssiva
void soma1(){

    return soma1();

}


void soma2(){
    
}


void soma3(){
    
}

#include <stdio.h>
#include <sys/time.h>

#define SIZE 1000000

// Alocação estática dos vetores
int a[SIZE];
int b[SIZE];

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    double start, end;
    int sum;
    
    // 1. Inicialização com cálculo simples
    start = get_time();
    for (int i = 0; i < SIZE; i++) {
        a[i] = i+1;
    }
    end = get_time();

    printf("Tempo inicializacao: %.6f segundos\n", end - start);
    
    // 2. Soma cumulativa com dependência
    sum = 0;
    start = get_time();
    for (int i = 0; i < SIZE; i++) {
        sum += a[i];
    }
    end = get_time();
    printf("Tempo soma cumulativa (1 var): %.6f segundos\n", end - start);
    printf("Valor da Soma: %d\n", sum);
    
    // 3. Soma com múltiplas variáveis (quebrando dependências)
    int sum1 = 0, sum2 = 0;
    start = get_time();
    for (int i = 0; i < SIZE; i += 2) {
        sum1 += a[i];
        sum2 += a[i+1];

    }
    sum = sum1 + sum2;
    end = get_time();
    printf("Tempo soma com 2 vars: %.6f segundos\n", end - start);
    printf("Valor da Soma: %d\n", sum);
    
    return 0;
}
/*
gcc -o ilp_experiment_O0 -O0 ilp_experiment.c
gcc -o ilp_experiment_O2 -O2 ilp_experiment.c
gcc -o ilp_experiment_O3 -O3 ilp_experiment.c*/