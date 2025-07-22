import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Dados
data = {
    "Tamanho": [1000, 2000, 4000, 8000],
    "Células (milhões)": [1, 4, 16, 64],
    "Tempo_Solucao": [0.0453, 0.0397, 0.0458, 0.1335],
    "Throughput": [220.59, 1006.34, 3489.78, 4793.58],
    "Erro_L2": [3.808794e-10, 1.540011e-10, 4.815913e-12, 1.499020e-10],
    "Valor_r": [0.0050, 0.0200, 0.0800, 0.3201],
    "Transfer_HtoD": [4.31, 17.28, 69.50, 191.36],
    "Transfer_DtoH": [1.34, 5.33, 19.02, 88.21],
    "Kernel_Time": [0.24, 0.86, 3.33, 13.16],
    "Estabilidade": ["Estável", "Estável", "Estável", "Estável"]
}
df = pd.DataFrame(data)

# Configurações de estilo
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'figure.figsize': (16, 10),
    'font.size': 12,
    'axes.titlesize': 14,
    'axes.labelsize': 12,
    'xtick.labelsize': 11,
    'ytick.labelsize': 11,
    'legend.fontsize': 11,
})

# Função utilitária para anotação
def anotar_pontos(ax, x, y):
    for xi, yi in zip(x, y):
        if pd.notna(yi):
            ax.annotate(f'{yi:.2f}', (xi, yi), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9)

# Gráfico 1: Tempo de Solução
fig, ax = plt.subplots()
ax.plot(df["Tamanho"], df["Tempo_Solucao"], 'o-', color='tab:blue', linewidth=2)
ax.set_title("Tempo de Solução vs Tamanho da Grade")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("Tempo (s)")
anotar_pontos(ax, df["Tamanho"], df["Tempo_Solucao"])
plt.tight_layout()
plt.show()

# Gráfico 2: Throughput
fig, ax = plt.subplots()
ax.plot(df["Tamanho"], df["Throughput"], 'o-', color='tab:green', linewidth=2)
ax.set_title("Throughput vs Tamanho da Grade")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("Throughput (Million Cells/sec)")
anotar_pontos(ax, df["Tamanho"], df["Throughput"])
plt.tight_layout()
plt.show()

# Gráfico 3: Erro L2 (escala log)
fig, ax = plt.subplots()
ax.semilogy(df["Tamanho"], df["Erro_L2"], 'o-', color='tab:red', linewidth=2)
ax.set_title("Erro L2 vs Tamanho da Grade (escala log)")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("Erro L2")
plt.tight_layout()
plt.show()

# Gráfico 4: Valor de r
fig, ax = plt.subplots()
ax.plot(df["Tamanho"], df["Valor_r"], 'o-', color='tab:purple', linewidth=2)
ax.set_title("Valor de r vs Tamanho da Grade")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("r = α·dt / dx²")
anotar_pontos(ax, df["Tamanho"], df["Valor_r"])
plt.tight_layout()
plt.show()

# Gráfico 5: Transferência HtoD e DtoH
fig, ax = plt.subplots()
valid_idx = df["Transfer_HtoD"].notna()
x = df["Tamanho"][valid_idx]
htod = df["Transfer_HtoD"][valid_idx]
dtoh = df["Transfer_DtoH"][valid_idx]

ax.plot(x, htod, 'o-', label='Host to Device (HtoD)', linewidth=2, color='tab:blue')
ax.plot(x, dtoh, 's--', label='Device to Host (DtoH)', linewidth=2, color='tab:orange')

ax.set_title("Tempo de Transferência entre CPU e GPU")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("Tempo (ms)")
ax.legend(loc='upper left')
anotar_pontos(ax, x, htod)
anotar_pontos(ax, x, dtoh)
plt.tight_layout()
plt.show()

# Gráfico 6: Kernel Time
fig, ax = plt.subplots()
x_kernel = df["Tamanho"][df["Kernel_Time"].notna()]
y_kernel = df["Kernel_Time"].dropna()
ax.plot(x_kernel, y_kernel, 'o-', color='tab:cyan', linewidth=2)
ax.set_title("Tempo de Execução do Kernel GPU")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_ylabel("Tempo (ms)")
anotar_pontos(ax, x_kernel, y_kernel)
plt.tight_layout()
plt.show()

# Gráfico 7: Estabilidade
fig, ax = plt.subplots()
colors = ['green' if est == "Estável" else 'red' for est in df["Estabilidade"]]
ax.bar(df["Tamanho"], [1]*len(df), color=colors)
ax.set_title("Estabilidade Numérica")
ax.set_xlabel("Tamanho da Grade (n x n)")
ax.set_yticks([])
for i, est in enumerate(df["Estabilidade"]):
    ax.text(df["Tamanho"][i], 1.02, est, ha='center', fontsize=11)
plt.tight_layout()
plt.show()