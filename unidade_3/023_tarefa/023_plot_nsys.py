import pandas as pd
import matplotlib.pyplot as plt

# Dados extraídos do profile NSYS (valores em segundos para facilitar visualização)
data = {
    'Categoria': [
        'CPU OS Runtime', 'CPU OS Runtime', 
        'CUDA API', 'CUDA API', 'CUDA API',
        'CUDA GPU Kernel', 'CUDA GPU Memcpy', 'CUDA GPU Memcpy'
    ],
    'Nome': [
        'sem_wait', 'poll',
        'cudaDeviceSynchronize', 'cudaMalloc', 'cudaMemcpy',
        'atualiza', 'Device-to-Host memcpy', 'Host-to-Device memcpy'
    ],
    'Total_Time_s': [
        2_903_207_891 / 1e9, 2_809_199_961 / 1e9,
        2_494_040_983 / 1e9, 212_038_257 / 1e9, 63_082_994 / 1e9,
        2_484_247_638 / 1e9, 22_626_697 / 1e9, 16_345_547 / 1e9
    ],
    'Time_percent': [
        48.0, 47.0,
        88.0, 7.0, 2.0,
        100.0, 58.0, 41.0
    ],
    'Num_Calls': [
        5, 41,
        2000, 2, 2002,
        2, 2001, 1
    ]
}

df = pd.DataFrame(data)

# Visualização 1: Barras de tempo total (s) por categoria e nome
plt.figure(figsize=(12, 6))
bars = plt.bar(df['Nome'], df['Total_Time_s'], color='cornflowerblue')
plt.xticks(rotation=45, ha='right')
plt.ylabel('Tempo total (s)')
plt.title('Tempo total gasto em operações no perfil NSYS')
plt.grid(axis='y')

# Mostrar valores em cima das barras
for bar in bars:
    height = bar.get_height()
    plt.annotate(f'{height:.2f}s',
                 xy=(bar.get_x() + bar.get_width() / 2, height),
                 xytext=(0, 3),  # 3 points vertical offset
                 textcoords="offset points",
                 ha='center', va='bottom')

plt.tight_layout()
plt.show()

# Visualização 2: Pie chart dos % tempos das principais operações CUDA (kernel + memcpy + sincronização)
cuda_ops = df[df['Categoria'].str.contains('CUDA')].copy()
plt.figure(figsize=(8, 8))
plt.pie(cuda_ops['Time_percent'], labels=cuda_ops['Nome'], autopct='%1.1f%%', startangle=140)
plt.title('Distribuição percentual de tempo nas operações CUDA')
plt.show()
