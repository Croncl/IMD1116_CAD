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

void calcular_pi_bbp(int iteracoes, long *tempo_ns, int *precisao, double *pi_aproximado) {
    double pi = 0.0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int k = 0; k < iteracoes; k++) {
        pi += (1.0/pow(16, k)) * (
              4.0/(8*k + 1) - 
              2.0/(8*k + 4) - 
              1.0/(8*k + 5) - 
              1.0/(8*k + 6));
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    *tempo_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    *precisao = digitos_corretos(pi);
    *pi_aproximado = pi;
}

void imprimir_tabela_bbp(int iteracoes[], int tamanho) {
    printf("\n┌──────────────┬─────────────────┬────────────────────┬─────────────────────┐\n");
    printf("│ %-14s │ %-15s │ %-19s │ %-20s │\n", 
           "Iterações", "Tempo (ns)", "Dígitos Corretos", "π Aproximado");
    printf("├──────────────┼─────────────────┼────────────────────┼─────────────────────┤\n");
    
    for(int i = 0; i < tamanho; i++) {
        long tempo;
        int precisao;
        double pi;
        
        calcular_pi_bbp(iteracoes[i], &tempo, &precisao, &pi);
        
        printf("│ %-12d │ %-15ld │ %-18d │ %-19.15f │\n", 
               iteracoes[i], tempo, precisao, pi);
    }
    
    printf("└──────────────┴─────────────────┴────────────────────┴─────────────────────┘\n");
}

int main() {
    // Fewer iterations needed for BBP to reach 15 digits
    int testes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20};
    int num_testes = sizeof(testes)/sizeof(testes[0]);
    
    imprimir_tabela_bbp(testes, num_testes);
    
    return 0;
}

//gcc -o 003tarefa 003tarefa.c -lm