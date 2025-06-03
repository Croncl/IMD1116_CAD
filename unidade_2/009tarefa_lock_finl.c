#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

// Estrutura de nó da lista encadeada
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Função para liberar a memória de listas
void free_lists(Node** lists, int num_lists) {
    for (int i = 0; i < num_lists; i++) {
        Node* current = lists[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
}

// Função para contar os elementos de uma lista
int count_elements(Node* list) {
    int count = 0;
    Node* current = list;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Versão com 2 listas e regiões críticas nomeadas
void two_lists_with_critical(int N) {
    const int num_threads = 4;
    const int num_lists = 2;
    Node* lists[2] = {NULL, NULL};
    int thread_insertions[4][2] = {{0}}; // threads x listas

    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();

        // Distribuição balanceada das inserções
        int base = N / num_threads;
        int resto = N % num_threads;
        int quota = base + (tid < resto ? 1 : 0);

        for (int i = 0; i < quota; i++) {
            int value = rand() % 1000;
            int list_choice = rand() % 2; // Escolhe aleatoriamente entre lista 0 ou 1

            // Região crítica nomeada para proteger apenas a lista escolhida
            if (list_choice == 0) {
                #pragma omp critical(LISTA0)
                {
                    Node* new_node = (Node*)malloc(sizeof(Node));
                    new_node->data = value;
                    new_node->next = lists[0];
                    lists[0] = new_node;
                    thread_insertions[tid][0]++;
                }
            } else {
                #pragma omp critical(LISTA1)
                {
                    Node* new_node = (Node*)malloc(sizeof(Node));
                    new_node->data = value;
                    new_node->next = lists[1];
                    lists[1] = new_node;
                    thread_insertions[tid][1]++;
                }
            }
        }
    }

    printf("\n=== VERSÃO COM 2 LISTAS E REGIÕES CRÍTICAS ===\n");
    for (int i = 0; i < num_lists; i++) {
        printf("Lista %d tem %d elementos.\n", i + 1, count_elements(lists[i]));
    }

    // Exibe inserções por thread
    printf("\nInserções por thread em cada lista:\n");
    for (int t = 0; t < num_threads; t++) {
        printf("Thread %d: Lista 1 = %d, Lista 2 = %d\n", t, thread_insertions[t][0], thread_insertions[t][1]);
    }

    free_lists(lists, num_lists);
}

// Versão generalizada para K listas com locks explícitos
void generalized_with_locks(int N, int num_lists) {
    const int num_threads = 4;
    Node** lists = (Node**)calloc(num_lists, sizeof(Node*));
    omp_lock_t* locks = (omp_lock_t*)malloc(num_lists * sizeof(omp_lock_t));
    int thread_insertions[4] = {0};

    // Inicializa locks
    for (int i = 0; i < num_lists; i++) {
        omp_init_lock(&locks[i]);
    }

    #pragma omp parallel num_threads(num_threads) //4 threads
    {
        int tid = omp_get_thread_num();

        // Distribuição balanceada das inserções
        int base = N / num_threads;
        int resto = N % num_threads;
        int quota = base + (tid < resto ? 1 : 0);

        for (int i = 0; i < quota; i++) {
            int value = rand() % 1000;
            int list_choice = rand() % num_lists;

            // Uso de lock explícito para garantir exclusão mútua
            omp_set_lock(&locks[list_choice]);
            Node* new_node = (Node*)malloc(sizeof(Node));
            new_node->data = value;
            new_node->next = lists[list_choice];
            lists[list_choice] = new_node;
            omp_unset_lock(&locks[list_choice]);
            // Contagem thread-safe: cada thread atualiza SEU PRÓPRIO contador (índice tid exclusivo)
            // Não requer proteção adicional, pois não há compartilhamento entre threads
            thread_insertions[tid]++;
        }
    }

    printf("\n=== VERSÃO GENERALIZADA COM LOCKS EXPLÍCITOS ===\n");
    for (int i = 0; i < num_lists; i++) {
        printf("Lista %d tem %d elementos.\n", i + 1, count_elements(lists[i]));
    }

    // Inserções por thread
    printf("\nTotal de inserções por thread:\n");
    for (int t = 0; t < num_threads; t++) {
        printf("Thread %d: %d inserções\n", t, thread_insertions[t]);
    }

    for (int i = 0; i < num_lists; i++) {
        omp_destroy_lock(&locks[i]);
    }

    free_lists(lists, num_lists);
    free(lists);
    free(locks);
}

int main() {
    int N;
    int K;

    printf("===========================================\n");
    printf("TAREFA 9 - Inserção concorrente em listas\n");
    printf("===========================================\n");

    printf("Digite o número total de inserções (N): ");
    scanf("%d", &N);

    srand(time(NULL));
    two_lists_with_critical(N); // Parte 1 - apenas 2 listas

    printf("\nAgora, generalizando para K listas com locks explícitos.\n");
    printf("Digite o número de listas (K): ");
    scanf("%d", &K);

    generalized_with_locks(N, K); // Parte 2 - generalizada

    printf("\nExecução concluída.\n");
    return 0;
}
