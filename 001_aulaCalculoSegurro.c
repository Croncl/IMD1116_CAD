#include <stdio.h>
#include <math.h>  // Necessário para sqrt()

int main() {
    int l3_size = 3 * 1024 * 1024; // 3MB em bytes
    int tipo_dado = sizeof(int);    // 4 bytes para int
    
    int elementos_maximos = l3_size / tipo_dado;
    int n_maximo = (int)sqrt(elementos_maximos * 0.8); // 80% da L3
    
    printf("Para int: Matriz segura até %dx%d\n", n_maximo, n_maximo);
    return 0;
}

//gcc 001_aulaCalculoSegurro.c -o 001_aulaCalculoSegurro -lm