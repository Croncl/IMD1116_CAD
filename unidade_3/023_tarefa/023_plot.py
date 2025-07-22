import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import interp1d
import seaborn as sns

sns.set(style="whitegrid", context="talk")

# Carrega e prepara o arquivo result.csv
def carregar_e_preparar(nome_arquivo):
    df = pd.read_csv(nome_arquivo)
    df.columns = df.columns.str.strip().str.lower()
    df['tempo'] = pd.to_numeric(df['tempo'], errors='coerce')
    df['valor_central'] = pd.to_numeric(df['valor_central'], errors='coerce')
    df = df.dropna(subset=['tempo', 'valor_central'])
    df['tempo_acumulado'] = df['tempo'].cumsum()
    return df

df = carregar_e_preparar("result.csv")

# Interpola para suavizar
tempos_comuns = np.linspace(0, df['tempo_acumulado'].max(), 1000)
interp_func = interp1d(
    df['tempo_acumulado'], df['valor_central'],
    bounds_error=False,
    fill_value=(df['valor_central'].iloc[0], df['valor_central'].iloc[-1])
)
valores_interpolados = interp_func(tempos_comuns)

# Plot
plt.figure(figsize=(12, 6))
plt.plot(tempos_comuns, valores_interpolados,
         label=f"CUDA (Tempo Total: {df['tempo_acumulado'].iloc[-1]:.2f}s)",
         color='seagreen', linewidth=2.5)

# Marcar pontos a cada 500 passos
step = 500
altura = 0.005
for i in range(0, len(df), step):
    tempo = df["tempo_acumulado"].iloc[i]
    valor = df["valor_central"].iloc[i]
    plt.vlines(x=tempo, ymin=valor - altura, ymax=valor + altura, color='seagreen', alpha=0.5)

plt.xlabel("Tempo acumulado (s)")
plt.ylabel("Valor Central")

plt.title("Simulação CUDA da Equação de Difusão 3D\n"
          "#define NX 300 | NY 300 | NZ 300 | NSTEPS 2000\n",
          fontsize=13)

plt.legend()
plt.grid(True, linestyle='--', alpha=0.4)
plt.tight_layout()
plt.savefig("grafico_cuda_result.png", dpi=300, bbox_inches='tight')
plt.show()
