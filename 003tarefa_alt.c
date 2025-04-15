#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define PI_REF "3.14159265358979323846"

int digitos_corretos(double aproximacao) {
    char pi_aproximado[25];
    char pi_ref[25] = PI_REF;
    snprintf(pi_aproximado, 24, "%.15f", aproximacao);
    
    int corretos = 0;
    for(int i = 0; i < strlen(pi_ref) && i < strlen(pi_aproximado); i++) {
        if(pi_ref[i] == pi_aproximado[i] && pi_ref[i] != '.') {
            corretos++;
        } else if(pi_ref[i] != pi_aproximado[i]) {
            break;
        }
    }
    return corretos;
}

void calcular_pi(long n, long *tempo_ns, int *precisao, double *pi_aproximado) {
    double pi = 0.0;
    int sinal = 1;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(long i = 0; i < n; i++) {
        pi += sinal / (2.0 * i + 1);
        sinal *= -1;
    }
    pi *= 4;

    clock_gettime(CLOCK_MONOTONIC, &end);
    *tempo_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    *precisao = digitos_corretos(pi);
    *pi_aproximado = pi;
}

void imprimir_tabela(long iteracoes[], int tamanho) {
    printf("\n┌──────────────┬─────────────────┬────────────────────┬─────────────────────┐\n");
    printf("│ %-14s │ %-15s │ %-19s │ %-20s │\n", 
           "Iterações", "Tempo (ns)", "Dígitos Corretos", "π Aproximado");
    printf("├──────────────┼─────────────────┼────────────────────┼─────────────────────┤\n");
    
    for(int i = 0; i < tamanho; i++) {
        long tempo;
        int precisao;
        double pi;
        
        calcular_pi(iteracoes[i], &tempo, &precisao, &pi);
        
        printf("│ %-12ld │ %-15ld │ %-18d │ %-19.15f │\n", 
               iteracoes[i], tempo, precisao, pi);
    }
    
    printf("└──────────────┴─────────────────┴────────────────────┴─────────────────────┘\n");
}

int main() {
    long testes[] = {10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 10000000000L};
    int num_testes = sizeof(testes)/sizeof(testes[0]);
    
    imprimir_tabela(testes, num_testes);
    
    return 0;
}