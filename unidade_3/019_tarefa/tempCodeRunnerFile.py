import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter

# Dados experimentais da simulação em GPU
data = {
    "Tamanho": [1000, 2000, 4000, 8000],          # Grade n x n
    "Células (milhões)": [1, 4, 16, 64],          # n² / 1e6
    "Tempo_Solucao": [0.3547, 0.4207, 0.6938, 1.8097],  # Tempo total (s)
    "Throughput": [28.19, 95.09, 230.61, 353.66],      # M células/s
    "Erro_L2": [3.808794e-10, 1.540011e-10, 4.815913e-12, 1.499020e-10]  # Norma L2
}

df = pd.DataFrame(data)

# Configurações de estilo
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({'figure.dpi': 300, 'font.size': 11})
colors = ['#1f77b4', '#ff7f0e', '#d62728']

# GRÁFICO 1 – Tempo de execução
fig1, ax1 = plt.subplots(figsize=(16, 10))
ax1.plot(df["Tamanho"], df["Tempo_Solucao"], 'o-', color=colors[0],
         linewidth=2.5, markersize=8, label='Tempo de Execução')

ax1.set_title('Tempo de Execução vs Tamanho da Grade')
ax1.set_xlabel('Tamanho da Grade (n x n)')
ax1.set_ylabel('Tempo (segundos)')
ax1.set_xticks(df["Tamanho"])
ax1.grid(True, linestyle='--', alpha=0.6)
ax1.legend(loc='upper left')

# Rótulos em cada ponto (fonte menor e um pouco mais acima)
for _, row in df.iterrows():
    ax1.annotate(f'{row["Tempo_Solucao"]:.3f}s',
                 (row["Tamanho"], row["Tempo_Solucao"]),
                 textcoords="offset points", xytext=(0, 12),
                 ha='center', fontsize=7)

# Texto explicativo POSICIONADO MAIS PARA BAIXO E COM FONTE MENOR
ax1.text(0.5, -0.30,
         "Tempo necessário para resolver a equação do calor na GPU\n"
         "para diferentes tamanhos de grade.",
         transform=ax1.transAxes, fontsize=7, ha='center', va='top',
         bbox=dict(facecolor='white', alpha=0.85, edgecolor='gray'))

plt.subplots_adjust(bottom=0.32)   # margem inferior maior
plt.savefig('grafico_tempo_execucao.png', bbox_inches='tight')
plt.show()

# GRÁFICO 2 – Throughput
fig2, ax2 = plt.subplots(figsize=(16, 10))
bars = ax2.bar(df["Tamanho"], df["Throughput"], width=550, color=colors[1])

ax2.set_title('Throughput Computacional vs Tamanho da Grade')
ax2.set_xlabel('Tamanho da Grade (n x n)')
ax2.set_ylabel('Milhões de células/segundo')
ax2.set_xticks(df["Tamanho"])
ax2.grid(True, axis='y', linestyle='--', alpha=0.6)
ax2.legend(['Throughput'], loc='upper left')

# Rótulos no topo das barras (fonte menor e mais para cima)
for bar in bars:
    height = bar.get_height()
    ax2.annotate(f'{height:.1f}M',
                 xy=(bar.get_x() + bar.get_width()/2, height),
                 xytext=(0, 8), textcoords="offset points",
                 ha='center', fontsize=7)

# Texto explicativo mais para baixo e fonte menor
ax2.text(0.5, -0.30,
         "Quantidade de células processadas por segundo pela GPU.\n"
         "Mede o desempenho computacional.",
         transform=ax2.transAxes, fontsize=7, ha='center', va='top',
         bbox=dict(facecolor='white', alpha=0.85, edgecolor='gray'))

plt.subplots_adjust(bottom=0.32)
plt.savefig('grafico_throughput.png', bbox_inches='tight')
plt.show()

# GRÁFICO 3 – Erro L2
fig3, ax3 = plt.subplots(figsize=(16, 10))
ax3.plot(df["Tamanho"], df["Erro_L2"], 's--', color=colors[2],
         linewidth=2.0, markersize=8, label='Erro L2')

ax3.set_title('Precisão Numérica vs Tamanho da Grade')
ax3.set_xlabel('Tamanho da Grade (n x n)')
ax3.set_ylabel('Erro (Norma L2)')
ax3.set_xticks(df["Tamanho"])
ax3.grid(True, linestyle='--', alpha=0.6)
ax3.legend(loc='upper right')
ax3.yaxis.set_major_formatter(ScalarFormatter())

# Rótulos nos pontos (fonte menor e mais para cima)
for _, row in df.iterrows():
    ax3.annotate(f'{row["Erro_L2"]:.1e}',
                 (row["Tamanho"], row["Erro_L2"]),
                 textcoords="offset points", xytext=(0, 12),
                 ha='center', fontsize=7)

# Texto explicativo mais para baixo e fonte menor
ax3.text(0.5, -0.30,
         "Erro da solução numérica comparado a uma referência exata.\n"
         "Mostra que a precisão é mantida em grades maiores.",
         transform=ax3.transAxes, fontsize=7, ha='center', va='top',
         bbox=dict(facecolor='white', alpha=0.85, edgecolor='gray'))

plt.subplots_adjust(bottom=0.32)
plt.savefig('grafico_erro_L2.png', bbox_inches='tight')
plt.show()
