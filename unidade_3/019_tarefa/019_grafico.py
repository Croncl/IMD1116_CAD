import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter

data = {
    "Tamanho": [1000, 2000, 4000, 8000],
    "Células (milhões)": [1, 4, 16, 64],
    "Tempo_Solucao": [0.3547, 0.4207, 0.6938, 1.8097],
    "Throughput": [28.19, 95.09, 230.61, 353.66],
    "Erro_L2": [3.808794e-10, 1.540011e-10, 4.815913e-12, 1.499020e-10]
}

df = pd.DataFrame(data)

plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({'figure.dpi': 300, 'font.size': 9})  # fonte geral um pouco menor
colors = ['#1f77b4', '#ff7f0e', '#d62728']

# Função para plotar e ajustar os gráficos de forma consistente
def plot_graph(x, y, kind='line', ylabel='', title='', colors_idx=0, label_fmt='{:.3f}', label_offset=2, ylim_multiplier=1.2):
    fig, ax = plt.subplots(figsize=(14, 8))
    if kind == 'line':
        ax.plot(x, y, 'o-', color=colors[colors_idx], linewidth=2.5, markersize=6)
    elif kind == 'bar':
        bars = ax.bar(x, y, width=550, color=colors[colors_idx])
    else:
        raise ValueError('kind deve ser "line" ou "bar"')

    ax.set_title(title, fontsize=12)
    ax.set_xlabel('Tamanho da Grade (n x n)', fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.set_xticks(x)
    ax.grid(True, linestyle='--', alpha=0.6)

    # Ajusta limite y com sobra customizável
    ylim_max = max(y) * ylim_multiplier
    ax.set_ylim(0, ylim_max)

    if kind == 'line':
        for xi, yi in zip(x, y):
            ax.annotate(label_fmt.format(yi),
                        (xi, yi),
                        textcoords="offset points", xytext=(0, label_offset),
                        ha='center', fontsize=7, color='black')
    else:  # barras
        for bar in bars:
            height = bar.get_height()
            ax.annotate(label_fmt.format(height),
                        xy=(bar.get_x() + bar.get_width() / 2, height),
                        xytext=(0, label_offset), textcoords="offset points",
                        ha='center', fontsize=7, color='black')

    # Texto explicativo menor e mais distante do gráfico
    ax.text(0.5, -0.40,
            'Tempo necessário para resolver a equação do calor na GPU\n'
            'para diferentes tamanhos de grade.' if ylabel.startswith('Tempo') else
            'Quantidade de células processadas por segundo pela GPU.\nMede o desempenho computacional.' if ylabel.startswith('Milhões') else
            'Erro da solução numérica comparado a uma referência exata.\nMostra que a precisão é mantida em grades maiores.',
            transform=ax.transAxes, fontsize=6, ha='center', va='top',
            bbox=dict(facecolor='white', alpha=0.85, edgecolor='gray'))
    plt.subplots_adjust(bottom=0.30, top=0.83)
    plt.show()


# Gráfico 1: Tempo
plot_graph(df["Tamanho"], df["Tempo_Solucao"], kind='line',
           ylabel='Tempo (segundos)', title='Tempo de Execução vs Tamanho da Grade',
           colors_idx=0, label_fmt='{:.3f}s')

# Gráfico 2: Throughput
plot_graph(df["Tamanho"], df["Throughput"], kind='bar',
           ylabel='Milhões de células/segundo', title='Throughput Computacional vs Tamanho da Grade',
           colors_idx=1, label_fmt='{:.1f}M')

# Gráfico 3: Erro L2 (com ylim_multiplier maior para evitar corte do label)
plot_graph(df["Tamanho"], df["Erro_L2"], kind='line',
           ylabel='Erro (Norma L2)', title='Precisão Numérica vs Tamanho da Grade',
           colors_idx=2, label_fmt='{:.1e}', ylim_multiplier=2.5)
