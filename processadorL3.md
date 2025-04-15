```markdown
# Cálculo do Tamanho Máximo de Matriz/Vetor para Cache L3 de 3MB

## 1. Cálculo Básico
- **Tamanho da L3**: 3 MB = 3 × 1024 × 1024 = **3.145.728 bytes**
- **Tipos de dados**:
  - `int`: 4 bytes
  - `double`: 8 bytes

### Elementos Máximos:
| Tipo de Dado | Cálculo                   | Elementos Máximos |
|--------------|---------------------------|-------------------|
| `int`        | 3.145.728 ÷ 4 =           | 786.432           |
| `double`     | 3.145.728 ÷ 8 =           | 393.216           |

## 2. Tamanho Máximo para Matriz × Vetor
**Dados em uso**:
- Matriz: m × n elementos
- Vetor: n elementos
- Resultado: m elementos

**Regra prática**:  
`matriz + vetor + resultado ≤ tamanho_L3`

### Para `int` (matriz quadrada N×N):
```
N² (matriz) + N (vetor) + N (resultado) ≤ 786.432
N² + 2N - 786.432 ≤ 0
```
**Solução**: N ≈ √786.432 ≈ 886  
**Matriz máxima**: 886×886  
**Elementos totais**: 784.996 (matriz) + 886 (vetor) + 886 (resultado) = 786.768

### Para `double`:
```
N² + 2N ≤ 393.216 → N ≈ 627
```
**Matriz máxima**: 627×627

## 3. Limitações Práticas
Fatores que reduzem o limite real:
- Uso do SO e outros aplicativos
- Dados adicionais na cache
- Associatividade da cache

**Recomendação segura** (80% da L3):

| Tipo de Dado | Tamanho Teórico | Tamanho Prático |
|--------------|-----------------|-----------------|
| `int`        | 886×886         | 800×800         |
| `double`     | 627×627         | 560×560         |

## 4. Código de Verificação
```c
#include <stdio.h>
#include <math.h>

int main() {
    int l3_size = 3 * 1024 * 1024; // 3MB em bytes
    int tipo_dado = sizeof(int);    // 4 bytes para int
    
    int elementos_maximos = l3_size / tipo_dado;
    int n_maximo = (int)sqrt(elementos_maximos * 0.8); // 80% da L3
    
    printf("Para int: Matriz segura até %dx%d\n", n_maximo, n_maximo);
    return 0;
}
```

## 5. Otimização para Matrizes Grandes
Técnicas recomendadas:
- **Blocking/Tiling**:
  ```c
  #define BLOCK_SIZE 64
  for (int i = 0; i < N; i += BLOCK_SIZE) {
      for (int j = 0; j < N; j += BLOCK_SIZE) {
          // Processar blocos pequenos
      }
  }
  ```
- Algoritmos cache-aware

> **Nota**: Valores baseados no Intel i5-3210M (3MB L3). Para outros processadores, ajuste o tamanho da L3.
``` 

Você pode copiar este markdown diretamente para um arquivo `.md`. Ele inclui:
- Tabelas comparativas
- Fórmulas matemáticas
- Blocos de código
- Destaques importantes