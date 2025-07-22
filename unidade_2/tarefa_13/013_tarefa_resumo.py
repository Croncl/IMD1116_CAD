import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Força backend sem GUI
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Estilos de plotagem
try:
    plt.style.use('seaborn-v0_8')  # Para versões recentes
except:
    try:
        plt.style.use('seaborn')  # Alternativa
    except:
        plt.style.use('ggplot')  # Reserva

sns.set_theme()  # Tema seaborn

def carregar_dados_completos(nome_arquivo):
    """Carrega o arquivo CSV completo gerado pelo programa C"""
    try:
        df = pd.read_csv(nome_arquivo)
        print(f"✅ Dados carregados de {nome_arquivo}!")
        print(f"Colunas disponíveis: {df.columns.tolist()}")
        return df
    except Exception as e:
        print(f"❌ Erro ao carregar {nome_arquivo}: {e}")
        return None

def processar_dados(df):
    """Processa os dados para análise"""
    if df is None:
        return None
    
    tempo_cols = [col for col in df.columns if col.startswith('Tempo_')]
    if not tempo_cols:
        print("❌ Nenhuma coluna de tempo encontrada!")
        return None
    
    affinity_threads = [col.replace('Tempo_', '') for col in tempo_cols]
    dfs = []
    
    for col, config in zip(tempo_cols, affinity_threads):
        try:
            affinity, threads = config.split('_')
            temp_df = pd.DataFrame({
                'Passo': df['Passo'],
                'ValorCentral': df['ValorCentral'],
                'Tempo': df[col],
                'Config': config,
                'Afinidade': affinity,
                'Threads': int(threads)
            })
            dfs.append(temp_df)
        except Exception as e:
            print(f"❌ Erro ao processar coluna {col}: {e}")
    
    if not dfs:
        return None
    
    return pd.concat(dfs).reset_index(drop=True)

def plot_graficos_combinados(df_completo, N):
    if df_completo is None:
        print("⚠️ Dados incompletos para plotar gráficos combinados")
        return
    
    thread_counts = sorted(df_completo['Threads'].unique())
    affinity_types = df_completo['Afinidade'].unique()
    
    palette = {
        'false': 'red',
        'true': 'blue',
        'close': 'green',
        'spread': 'purple',
        'master': 'orange'
    }
    
    fig, axes = plt.subplots(len(thread_counts), len(affinity_types), 
                             figsize=(20, 15), sharex=True, sharey=True)
    fig.suptitle(f'Análise Completa de Desempenho (N={N})', fontsize=16)
    
    for i, threads in enumerate(thread_counts):
        for j, affinity in enumerate(affinity_types):
            ax = axes[i, j]
            df_sub = df_completo[(df_completo['Threads'] == threads) & 
                                 (df_completo['Afinidade'] == affinity)]
            if not df_sub.empty:
                df_sub = df_sub.sort_values('Passo')
                df_sub['TempoAcumulado'] = df_sub['Tempo'].cumsum()
                
                ax.plot(df_sub['TempoAcumulado'], df_sub['ValorCentral'],
                        color=palette.get(affinity, 'gray'), linewidth=1.5)
                
                if i == len(thread_counts) - 1:
                    ax.set_xlabel('Tempo (s)')
                if j == 0:
                    ax.set_ylabel(f'{threads}T\nValor Central')
                
                ax.set_title(f'Afinidade: {affinity}')
                ax.grid(True)
    
    plt.tight_layout()
    nome_saida = f'013_analise_completa_N{N}.png'
    plt.savefig(nome_saida, dpi=300, bbox_inches='tight')
    print(f"✅ Gráfico combinado salvo como: {nome_saida}")
    plt.close()

def plot_tempo_total(df_completo, N):
    if df_completo is None:
        print("⚠️ Dados incompletos para plotar tempos totais")
        return
    
    df_summary = df_completo.groupby(['Afinidade', 'Threads'])['Tempo'].sum().reset_index()
    
    plt.figure(figsize=(12, 6))
    palette = {
        'false': 'red',
        'true': 'blue',
        'close': 'green',
        'spread': 'purple',
        'master': 'orange'
    }
    
    for affinity in df_summary['Afinidade'].unique():
        df_aff = df_summary[df_summary['Afinidade'] == affinity]
        plt.plot(df_aff['Threads'], df_aff['Tempo'], 'o-',
                 label=affinity, color=palette.get(affinity, 'gray'),
                 markersize=8, linewidth=2)
    
    plt.title(f'Tempo Total de Execução por Configuração (N={N})')
    plt.xlabel('Número de Threads')
    plt.ylabel('Tempo Total (s)')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    
    filename = f'013_tempo_total_execucao_N{N}.png'
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"✅ Gráfico de tempo total salvo como: {filename}")
    plt.close()

def main():
    print("📊 Iniciando análise dos dados...\n")
    print("📁 Diretório atual:", os.getcwd())
    
    tamanhos_N = [100]
    
    for N in tamanhos_N:
        print(f"\n🔍 Analisando dados para N = {N}...")
        nome_arquivo = f'013_resultados_completosN{N}.csv'
        
        if not os.path.exists(nome_arquivo):
            print(f"❌ Arquivo {nome_arquivo} não encontrado!")
            continue
        
        df = carregar_dados_completos(nome_arquivo)
        if df is None:
            continue
        
        df_completo = processar_dados(df)
        if df_completo is None:
            continue
        
        print(f"✅ Dados processados com sucesso para N={N}!")
        print(f"Configurações encontradas: {df_completo['Config'].unique()}")
        
        plot_graficos_combinados(df_completo, N)
        plot_tempo_total(df_completo, N)

if __name__ == "__main__":
    main()
