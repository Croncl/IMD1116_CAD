#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>  // Adicionado para clock_gettime()

// Estrutura para representar um nó da lista encadeada
typedef struct Node {
    char filename[50];  // Nome do arquivo fictício
    struct Node* next;  // Ponteiro para o próximo nó
} Node;

// Função para criar um novo nó
Node* create_node(const char* filename) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Erro de alocação de memória\n");
        exit(EXIT_FAILURE);
    }
    snprintf(new_node->filename, 50, "%s", filename);
    new_node->next = NULL;
    return new_node;
}

// Função para adicionar um nó ao final da lista
void append_node(Node** head, const char* filename) {
    Node* new_node = create_node(filename);
    if (!*head) {
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next) current = current->next;
        current->next = new_node;
    }
}

// Função para processar um arquivo (simulado)
void process_file(const char* filename, int thread_id, int task_seq) {
    printf("Thread %d executou tarefa %d: %s\n", 
           thread_id, task_seq, filename);
    // Simula trabalho com tempo variável (para demonstrar paralelismo)
    for (volatile int i = 0; i < 100000000 * (1 + task_seq%3); i++) {}
}

// Função para liberar a memória da lista
void free_list(Node* head) {
    while (head) {
        Node* next = head->next;
        free(head);
        head = next;
    }
}

int main() {
    Node* head = NULL;
    const char* files[] = {
        "documento1.txt", "imagem.jpg", "dados.csv", "relatorio.pdf",
        "apresentacao.pptx", "config.ini", "script.py", "log.txt"
    };

    // Inicializa lista com 8 arquivos fictícios
    for (int i = 0; i < 8; i++) {
        append_node(&head, files[i]);
    }

    struct timespec start, end;
    double elapsed;

    /* 
     * VERSÃO RECOMENDADA: single com nowait
     */
    printf("\n=== VERSÃO RECOMENDADA (single com nowait) ===\n");
    clock_gettime(CLOCK_REALTIME, &start);
    
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            Node* current = head;
            int task_seq = 0;
            while (current) {
                int local_seq = ++task_seq;
                #pragma omp task firstprivate(current, local_seq)
                {
                    process_file(current->filename, omp_get_thread_num(), local_seq);
                }
                current = current->next;
            }
        }
    }
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", elapsed);

    /* 
     * VERSÃO ALTERNATIVA: single sem nowait
     */
    printf("\n=== VERSÃO ALTERNATIVA (single sem nowait) ===\n");
    clock_gettime(CLOCK_REALTIME, &start);
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            Node* current = head;
            int task_seq = 0;
            while (current) {
                int local_seq = ++task_seq;
                #pragma omp task firstprivate(current, local_seq)
                {
                    process_file(current->filename, omp_get_thread_num(), local_seq);
                }
                current = current->next;
            }
        }
    }
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", elapsed);

    /* 
     * VERSÃO NÃO RECOMENDADA: usando master
     */
    printf("\n=== VERSÃO NÃO RECOMENDADA (usando master) ===\n");
    clock_gettime(CLOCK_REALTIME, &start);
    
    #pragma omp parallel
    {
        #pragma omp master
        {
            Node* current = head;
            int task_seq = 0;
            while (current) {
                int local_seq = ++task_seq;
                #pragma omp task firstprivate(current, local_seq)
                {
                    process_file(current->filename, omp_get_thread_num(), local_seq);
                }
                current = current->next;
            }
        }
    }
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Tempo: %.4f segundos\n", elapsed);

    // Libera a memória alocada para a lista
    free_list(head);
    return 0;
}
//gcc -o 005_tarefa_copy 005_tarefa_conta_primos_copy.c  -fopenmp -lm -lrt 