import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import interp1d

def carregar_e_limpar(nome_arquivo):
    df = pd.read_csv(nome_arquivo)
    df['Tempo'] = pd.to_numeric(df['Tempo'], errors='coerce')
    df['ValorCentral'] = pd.to_numeric(df['ValorCentral'], errors='coerce')
    df = df.dropna(subset=['Tempo', 'ValorCentral'])
    df['TempoAcumulado'] = df['Tempo'].cumsum()
    return df

# Carrega os 3 arquivos necessários (CUDA + 32t + 64t)
df_cuda = carregar_e_limpar("simulacao_cuda.csv")
df_static32 = carregar_e_limpar("simulacao_npad32t.csv")
df_static64 = carregar_e_limpar("simulacao_npad64t.csv")  # Adicione seu arquivo de 64 threads aqui

# Determina os tempos máximos e valores
max_tempo = max(df_cuda['TempoAcumulado'].max(), 
                df_static32['TempoAcumulado'].max(),
                df_static64['TempoAcumulado'].max()) * 1.03

min_valor = min(df_cuda['ValorCentral'].min(), 
               df_static32['ValorCentral'].min(),
               df_static64['ValorCentral'].min()) * 0.97

max_valor = max(df_cuda['ValorCentral'].max(), 
               df_static32['ValorCentral'].max(),
               df_static64['ValorCentral'].max())

# Eixo de tempo comum para interpolação
tempos_comuns = np.linspace(0, max_tempo, 1000)

# Função de interpolação
def interpolar_com_limite(df, tempos_comuns):
    max_tempo_real = df['TempoAcumulado'].max()
    interp_func = interp1d(
        df['TempoAcumulado'], df['ValorCentral'],
        bounds_error=False,
        fill_value=(df['ValorCentral'].iloc[0], df['ValorCentral'].iloc[-1])
    )
    valores = interp_func(tempos_comuns)
    valores[tempos_comuns > max_tempo_real] = np.nan
    return valores

# Interpola os dados
valores_cuda = interpolar_com_limite(df_cuda, tempos_comuns)
valores_static32 = interpolar_com_limite(df_static32, tempos_comuns)
valores_static64 = interpolar_com_limite(df_static64, tempos_comuns)

# Tempos finais de execução
tempo_cuda = df_cuda["TempoAcumulado"].iloc[-1]
tempo_static32 = df_static32["TempoAcumulado"].iloc[-1]
tempo_static64 = df_static64["TempoAcumulado"].iloc[-1]

# Cálculo de Speedup (CUDA como referência)
speedup32 = tempo_static32 / tempo_cuda
speedup64 = tempo_static64 / tempo_cuda

# Plot
plt.figure(figsize=(14, 7))

# Linhas interpoladas (AGORA COM TEMPO TOTAL NAS LEGENDAS)
plt.plot(tempos_comuns, valores_cuda, 
         label=f'CUDA (Tempo Total: {tempo_cuda:.2f}s)', color='green', linewidth=3, linestyle='-')
plt.plot(tempos_comuns, valores_static32, 
         label=f'OpenMP Static - 32 thr (Speedup: {speedup32:.2f}x | Tempo: {tempo_static32:.2f}s)', color='blue', linewidth=2)
plt.plot(tempos_comuns, valores_static64, 
         label=f'OpenMP Static - 64 thr (Speedup: {speedup64:.2f}x | Tempo: {tempo_static64:.2f}s)', color='red', linewidth=2)

# Marcadores a cada 500 passos (apenas para OpenMP)
step = 500
height = 0.005
for df, color in zip([df_static32, df_static64], ['blue', 'red']):
    for i in range(0, len(df), step):
        tempo = df["TempoAcumulado"].iloc[i]
        plt.vlines(x=tempo, ymin=df["ValorCentral"].iloc[i] - height,
                   ymax=df["ValorCentral"].iloc[i] + height, color=color, alpha=0.5)

plt.xlabel("Tempo acumulado (s)", fontsize=12)
plt.ylabel("Valor Central", fontsize=12)
plt.title("Comparação CUDA vs OpenMP Static (32 e 64 threads)", fontsize=14)
plt.ylim(min_valor, max_valor)
plt.xlim(0, max_tempo)
plt.legend(fontsize=10, loc='upper right')
plt.grid(True, linestyle='--', alpha=0.5)
plt.tight_layout()
plt.savefig("comparacao_cuda_vs_static32_64.png", dpi=300, bbox_inches='tight')
plt.show()