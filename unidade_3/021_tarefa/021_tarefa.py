import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Dados
data = {
    "problem_size": ["small","small","small","small","small","small",
                     "medium","medium","medium","medium","medium","medium",
                     "large","large","large"],
    "nodes": [1,2,2,1,2,1,1,2,1,2,1,2,2,2,2],
    "processes": [4,4,8,2,2,1,4,4,2,2,1,8,4,8,2],
    "threads_per_process": [8,4,8,4,2,2,8,4,4,2,2,8,4,8,2],
    "total_time": [0.583765,1.099163,0.475924,3.264892,4.363787,8.008795,
                   1.836733,3.587617,11.753369,18.453578,33.674202,1.212420,
                   13.502725,28.938489,64.783935],
    "computation_time": [0.550607,0.852732,0.315341,3.189794,4.103122,8.007075,
                         1.731650,3.271748,11.607042,17.283304,33.666787,0.952984,
                         13.009292,16.300380,63.846391],
    "communication_time": [0.012013,0.114620,0.072803,0.017256,0.139180,0.0,
                           0.016069,0.124940,0.019397,0.679212,0.0,0.113947,
                           0.139788,0.149700,0.469253]
}

df = pd.DataFrame(data)

# Ordenar dados
order_problem_size = ["small", "medium", "large"]
df["problem_size"] = pd.Categorical(df["problem_size"], categories=order_problem_size, ordered=True)
df = df.sort_values(by=["problem_size", "nodes", "processes"]).reset_index(drop=True)

# --- Calcular speedup e eficiencia ---
# Considera como base o tempo do menor número de processos para cada tamanho de problema
speedup = []
eficiency = []

for size in order_problem_size:
    subset = df[df.problem_size == size]
    base_time = subset[subset.processes == subset.processes.min()]["total_time"].values[0]
    for _, row in subset.iterrows():
        sp = base_time / row["total_time"]
        ef = sp / row["processes"]
        speedup.append(sp)
        eficiency.append(ef)

df["speedup"] = speedup
df["efficiency"] = eficiency

# --- Gráficos ---

sns.set(style="whitegrid")
palette = sns.color_palette("Set2")

# Gráfico barras - Computation, Communication e Total
plt.figure(figsize=(14,6))
bar_width = 0.25
x = np.arange(len(df))

plt.bar(x - bar_width, df["computation_time"], width=bar_width, label="Computation Time", color=palette[0])
plt.bar(x, df["communication_time"], width=bar_width, label="Communication Time", color=palette[1])
plt.bar(x + bar_width, df["total_time"], width=bar_width, label="Total Time", color=palette[2])

plt.xticks(x, df.apply(lambda r: f'{r.problem_size}\nNodes:{r.nodes}\nProc:{r.processes}', axis=1), rotation=45, ha="right")
plt.ylabel("Time (s)")
plt.title("Comparison of Computation, Communication and Total Times")
plt.legend()
plt.tight_layout()
plt.show()

# Gráfico Speedup
plt.figure(figsize=(12,5))
sns.barplot(data=df, x=df.index, y="speedup", hue="problem_size", palette=palette)
plt.xticks(df.index, df.apply(lambda r: f'{r.problem_size}\nNodes:{r.nodes}\nProc:{r.processes}', axis=1), rotation=45, ha="right")
plt.ylabel("Speedup")
plt.title("Speedup by Configuration")
plt.legend(title="Problem Size")
plt.tight_layout()
plt.show()

# Gráfico Eficiência
plt.figure(figsize=(12,5))
sns.barplot(data=df, x=df.index, y="efficiency", hue="problem_size", palette=palette)
plt.xticks(df.index, df.apply(lambda r: f'{r.problem_size}\nNodes:{r.nodes}\nProc:{r.processes}', axis=1), rotation=45, ha="right")
plt.ylabel("Efficiency")
plt.title("Efficiency by Configuration")
plt.legend(title="Problem Size")
plt.tight_layout()
plt.show()

# --- Exibir tabela ordenada ---
print("\nTabela ordenada para relatório:\n")
print(df[["problem_size", "nodes", "processes", "threads_per_process", "total_time", "computation_time", "communication_time", "speedup", "efficiency"]].to_string(index=False))
