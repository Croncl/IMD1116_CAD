#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

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

    /* 
     * VERSÃO RECOMENDADA: single com nowait
     * - #pragma omp single: apenas uma thread executa o bloco
     * - nowait: as outras threads não esperam e podem começar a executar tarefas imediatamente
     * - firstprivate: cada tarefa recebe sua própria cópia das variáveis
     */
    printf("\n=== VERSÃO RECOMENDADA (single com nowait) ===\n");
    double start = omp_get_wtime();
    
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
        // Barreira implícita no final da região paralela garante que todas as tarefas sejam concluídas
    }
    printf("Tempo: %.4f segundos\n", omp_get_wtime() - start);

    /* 
     * VERSÃO ALTERNATIVA: single sem nowait
     * - Sem nowait, as outras threads esperam até que a thread única termine o bloco
     * - Isso pode reduzir o paralelismo no início
     */
    printf("\n=== VERSÃO ALTERNATIVA (single sem nowait) ===\n");
    start = omp_get_wtime();
    
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
    printf("Tempo: %.4f segundos\n", omp_get_wtime() - start);

    /* 
     * VERSÃO NÃO RECOMENDADA: usando master
     * - #pragma omp master: apenas a thread master (0) executa o bloco
     * - Não há garantia de que as outras threads estarão disponíveis para executar tarefas
     * - Pode levar a execução serial das tarefas
     */
    printf("\n=== VERSÃO NÃO RECOMENDADA (usando master) ===\n");
    start = omp_get_wtime();
    
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
    printf("Tempo: %.4f segundos\n", omp_get_wtime() - start);

    // Libera a memória alocada para a lista
    free_list(head);
    return 0;
}
//gcc -fopenmp 007.tarefa_copy.c -o 007.tarefa_copy