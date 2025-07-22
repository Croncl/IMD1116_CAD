import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Dados atualizados
data = {
    "problem_size": [
        "small","small","small","small","small","small",
        "medium","medium","medium","medium","medium","medium",
        "large","large","large",
        "small","small",
        "medium","medium",
        "large","large"
    ],
    "nodes": [
        1,2,2,1,2,1,
        1,2,1,2,1,2,
        2,2,2,
        4,4,
        4,4,
        4,4
    ],
    "processes": [
        4,4,8,2,2,1,
        4,4,2,2,1,8,
        4,8,2,
        8,4,
        8,4,
        8,4
    ],
    "threads_per_process": [
        8,4,8,4,2,2,
        8,4,4,2,2,8,
        4,8,2,
        4,2,
        4,2,
        4,2
    ],
    "total_time": [
        0.583765,1.099163,0.475924,3.264892,4.363787,8.008795,
        1.836733,3.587617,11.753369,18.453578,33.674202,1.212420,
        13.502725,28.938489,64.783935,
        0.995949,2.363831,
        2.536858,8.337660,
        9.060317,34.158938
    ],
    "computation_time": [
        0.550607,0.852732,0.315341,3.189794,4.103122,8.007075,
        1.731650,3.271748,11.607042,17.283304,33.666787,0.952984,
        13.009292,16.300380,63.846391,
        0.639805,1.986210,
        2.190665,7.658435,
        8.542502,32.634492
    ],
    "communication_time": [
        0.012013,0.114620,0.072803,0.017256,0.139180,0.0,
        0.016069,0.124940,0.019397,0.679212,0.0,0.113947,
        0.139788,0.149700,0.469253,
        0.175454,0.200614,
        0.185298,0.252683,
        0.251000,0.509967
    ]
}

df = pd.DataFrame(data)

# Ordenar
order_problem_size = ["small", "medium", "large"]
df["problem_size"] = pd.Categorical(df["problem_size"], categories=order_problem_size, ordered=True)
df = df.sort_values(by=["problem_size", "nodes", "processes"]).reset_index(drop=True)

# Calcular speedup e eficiência
speedup = []
efficiency = []

for size in order_problem_size:
    subset = df[df.problem_size == size]
    base_time = subset[subset.processes == subset.processes.min()]["total_time"].values[0]
    for _, row in subset.iterrows():
        sp = base_time / row["total_time"]
        ef = sp / row["processes"]
        speedup.append(sp)
        efficiency.append(ef)

df["speedup"] = speedup
df["efficiency"] = efficiency

# Exportar CSV
df.to_csv("tabela_resultados.csv", index=False)

# Labels eixo X
df["label"] = df.apply(lambda r: f'{r.problem_size}\nN:{r.nodes} P:{r.processes}', axis=1)

# Estilo visual
sns.set(style="whitegrid")
palette = sns.color_palette("Set2")

# Gráfico 1: Tempo de computação, comunicação e total
plt.figure(figsize=(18,6))
bar_width = 0.25
x = np.arange(len(df))

plt.bar(x - bar_width, df["computation_time"], width=bar_width, label="Computation", color=palette[0])
plt.bar(x, df["communication_time"], width=bar_width, label="Communication", color=palette[1])
plt.bar(x + bar_width, df["total_time"], width=bar_width, label="Total", color=palette[2])

plt.xticks(x, df["label"], rotation=45, ha="center")
plt.ylabel("Time (s)")
plt.title("Comparison of Computation, Communication, and Total Times")
plt.legend()
plt.tight_layout()
plt.show()

# Gráfico 2: Speedup
plt.figure(figsize=(16,5))
sns.barplot(x=df["label"], y=df["speedup"], hue=df["problem_size"], dodge=False, palette=palette)
plt.xticks(rotation=45, ha="center")
plt.ylabel("Speedup")
plt.title("Speedup by Configuration")
plt.legend(title="Problem Size")
plt.tight_layout()
plt.show()

# Gráfico 3: Eficiência
plt.figure(figsize=(16,5))
sns.barplot(x=df["label"], y=df["efficiency"], hue=df["problem_size"], dodge=False, palette=palette)
plt.xticks(rotation=45, ha="center")
plt.ylabel("Efficiency")
plt.title("Efficiency by Configuration")
plt.legend(title="Problem Size")
plt.tight_layout()
plt.show()
