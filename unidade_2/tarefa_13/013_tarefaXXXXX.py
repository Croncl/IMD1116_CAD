import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import interp1d
import seaborn as sns

# Configuração do estilo mais robusta
try:
    plt.style.use('seaborn-v0_8')  # Novo nome para versões mais recentes
except:
    try:
        plt.style.use('seaborn')  # Nome antigo
    except:
        plt.style.use('ggplot')  # Estilo alternativo

sns.set_theme()  # Inicializa o tema seaborn

def carregar_dados_completos(nome_arquivo):
    """Carrega o arquivo CSV completo gerado pelo programa C"""
    try:
        df = pd.read_csv(nome_arquivo)
        print("Dados carregados com sucesso!")
        print(f"Colunas disponíveis: {df.columns.tolist()}")
        return df
    except Exception as e:
        print(f"Erro ao carregar {nome_arquivo}: {e}")
        return None

def processar_dados(df):
    """Processa os dados para análise"""
    if df is None:
        return None
    
    # Extrai os nomes das colunas de tempo
    tempo_cols = [col for col in df.columns if col.startswith('Tempo_')]
    if not tempo_cols:
        print("Nenhuma coluna de tempo encontrada!")
        return None
    
    affinity_threads = [col.replace('Tempo_', '') for col in tempo_cols]
    
    # Cria um DataFrame limpo para cada configuração
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
            print(f"Erro ao processar coluna {col}: {e}")
    
    if not dfs:
        return None
    
    return pd.concat(dfs).reset_index(drop=True)

def plot_evolucao_por_threads(df_completo):
    """Plota a evolução temporal separada por número de threads"""
    if df_completo is None:
        print("Dados incompletos para plotar evolução temporal")
        return
    
    thread_counts = sorted(df_completo['Threads'].unique())
    affinity_types = df_completo['Afinidade'].unique()
    
    # Cores para diferentes afinidades
    palette = {
        'false': 'red',
        'true': 'blue',
        'close': 'green',
        'spread': 'purple',
        'master': 'orange'
    }
    
    # Cria um gráfico separado para cada contagem de threads
    for threads in thread_counts:
        plt.figure(figsize=(12, 6))
        df_sub = df_completo[df_completo['Threads'] == threads]
        
        # Calcula tempo acumulado para cada configuração
        for affinity in affinity_types:
            df_aff = df_sub[df_sub['Afinidade'] == affinity]
            df_aff = df_aff.sort_values('Passo')
            df_aff['TempoAcumulado'] = df_aff['Tempo'].cumsum()
            
            plt.plot(df_aff['TempoAcumulado'], df_aff['ValorCentral'], 
                    label=f'{affinity}', color=palette.get(affinity, 'gray'),
                    alpha=0.8, linewidth=2)
        
        plt.title(f'Evolução do Valor Central - {threads} Threads')
        plt.xlabel('Tempo Acumulado (s)')
        plt.ylabel('Valor Central')
        plt.grid(True)
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.tight_layout()
        
        filename = f'evolucao_{threads}threads.png'
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"Gráfico salvo como: {filename}")
        plt.close()

def plot_comparacao_afinidades(df_completo):
    """Plota comparação entre políticas de afinidade para cada thread"""
    if df_completo is None:
        print("Dados incompletos para plotar comparação")
        return
    
    affinity_types = df_completo['Afinidade'].unique()
    thread_counts = sorted(df_completo['Threads'].unique())
    
    # Cores para diferentes threads
    palette = sns.color_palette("husl", len(thread_counts))
    
    # Cria um gráfico separado para cada política de afinidade
    for affinity in affinity_types:
        plt.figure(figsize=(12, 6))
        df_aff = df_completo[df_completo['Afinidade'] == affinity]
        
        for i, threads in enumerate(thread_counts):
            df_thread = df_aff[df_aff['Threads'] == threads]
            df_thread = df_thread.sort_values('Passo')
            df_thread['TempoAcumulado'] = df_thread['Tempo'].cumsum()
            
            plt.plot(df_thread['TempoAcumulado'], df_thread['ValorCentral'], 
                    label=f'{threads} threads', color=palette[i],
                    alpha=0.8, linewidth=2)
        
        plt.title(f'Comparação de Threads - Afinidade {affinity}')
        plt.xlabel('Tempo Acumulado (s)')
        plt.ylabel('Valor Central')
        plt.grid(True)
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.tight_layout()
        
        filename = f'comparacao_threads_afinidade_{affinity}.png'
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"Gráfico salvo como: {filename}")
        plt.close()

def plot_tempo_total(df_completo):
    """Plota o tempo total de execução para cada configuração"""
    if df_completo is None:
        print("Dados incompletos para plotar tempos totais")
        return
    
    # Calcula tempo total para cada configuração
    df_summary = df_completo.groupby(['Afinidade', 'Threads'])['Tempo'].sum().reset_index()
    
    plt.figure(figsize=(12, 6))
    
    # Cores para diferentes afinidades
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
    
    plt.title('Tempo Total de Execução por Configuração')
    plt.xlabel('Número de Threads')
    plt.ylabel('Tempo Total (s)')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    
    filename = 'tempo_total_execucao.png'
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"Gráfico salvo como: {filename}")
    plt.close()

def main():
    print("Iniciando análise dos dados...")
    
    # Carrega os dados
    df = carregar_dados_completos('013_resultados_completosN200.csv')
    if df is None:
        print("Não foi possível carregar os dados. Verifique se o arquivo existe.")
        return
    
    df_completo = processar_dados(df)
    if df_completo is None:
        print("Não foi possível processar os dados.")
        return
    
    print("\nDados processados com sucesso!")
    print(f"Configurações encontradas: {df_completo['Config'].unique()}")
    
    # Gera os gráficos separados
    plot_evolucao_por_threads(df_completo)
    plot_comparacao_afinidades(df_completo)
    plot_tempo_total(df_completo)

if __name__ == '__main__':
    main()