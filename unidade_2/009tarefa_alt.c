//escreva um programa que cri tarefas par realizar N insercoes em duas listas encadeadas, cada uma associada a uma thread, Cada thread deve escolher aleatoriamente em qual lsita inserir um número. Garanta a integridade das lsitas evitando condcao de corrida e, sempre que possivel, use regioes criticas nomeadas(omp pragma critical nome) para que a insercao em uma lista nao bloqueie a outra. Em seguida, generalize o programa o numero de listas e ( que e igual ao numero de threads) definidos pelo usuario(em linha de comando). Explique por que, nesse caso, regioes criticas nomeadas nao sao suficientes e por que o uso de locks explicitos e torna necessario. Faca o codigo comparando com lock e critical.
// OBS:. sao mutex? ver. OBS2.: explicar pot que quando o numero de listas definidos pelo usuario nao funciona com critical ou pq nao e adequado...

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define MAX_LISTS 2  // Número máximo fixo de listas

typedef struct Node {
    int data;
    struct Node* next;
} Node;

int main(int argc, char* argv[]) {
    const int N = 100;
    Node* lists[MAX_LISTS] = {NULL};
    
    // Versão com critical nomeado
    #pragma omp parallel num_threads(MAX_LISTS)
    {
        unsigned seed = time(NULL) ^ omp_get_thread_num();
        
        for (int i = 0; i < N; i++) {
            int value = rand_r(&seed) % 1000;
            int list_choice = rand_r(&seed) % MAX_LISTS;
            
            if (list_choice == 0) {
                #pragma omp critical(list1_lock)
                {
                    Node* new_node = (Node*)malloc(sizeof(Node));
                    new_node->data = value;
                    new_node->next = lists[0];
                    lists[0] = new_node;
                }
            } else {
                #pragma omp critical(list2_lock)
                {
                    Node* new_node = (Node*)malloc(sizeof(Node));
                    new_node->data = value;
                    new_node->next = lists[1];
                    lists[1] = new_node;
                }
            }
        }
    }

    // Imprime e libera memória
    for (int i = 0; i < MAX_LISTS; i++) {
        printf("\nLista %d (%d elementos):\n", i, N);
        Node* current = lists[i];
        int count = 0;
        
        while (current != NULL) {
            // Imprime valor formatado em 5 caracteres
            printf("%5d ", current->data);
            count++;
            
            // Quebra de linha a cada 20 valores
            if (count % 20 == 0) {
                printf("\n");
            }
            
            Node* temp = current;
            current = current->next;
            free(temp);
        }
        
        // Quebra de linha final se não terminou em múltiplo de 20
        if (count % 20 != 0) {
            printf("\n");
        }
        
        printf("Total: %d elementos\n", count);
    }

    return 0;
}