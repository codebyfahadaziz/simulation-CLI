# Bonus: matplotlib charts. Run the simulator's "Compare ALL" first (creates results.csv, gantt.csv)
# Install: pip3 install matplotlib
import csv
import matplotlib.pyplot as plt

# 1) Bar graph: average waiting time per algorithm
names, wt = [], []
with open("results.csv") as f:
    for r in csv.DictReader(f):
        names.append(r["algorithm"]); wt.append(float(r["avg_wt"]))
plt.figure(figsize=(9, 4.5))
bars = plt.bar(names, wt, color="steelblue")
plt.bar_label(bars, fmt="%.2f")
plt.ylabel("Average waiting time"); plt.title("Average Waiting Time Comparison")
plt.xticks(rotation=20); plt.tight_layout()
plt.savefig("comparison_graph.png", dpi=150)

# 2) Gantt chart for every algorithm
data = {}
with open("gantt.csv") as f:
    for r in csv.DictReader(f):
        data.setdefault(r["algorithm"], []).append((int(r["pid"]), int(r["start"]), int(r["end"])))
fig, axes = plt.subplots(len(data), 1, figsize=(10, 1.6 * len(data)), sharex=True)
cmap = plt.get_cmap("tab10")
for ax, (alg, segs) in zip(axes, data.items()):
    for pid, s, e in segs:
        color = "lightgrey" if pid == -1 else cmap(pid % 10)
        ax.barh(0, e - s, left=s, color=color, edgecolor="black")
        if pid != -1 and e - s >= 1:
            ax.text((s + e) / 2, 0, f"P{pid}", ha="center", va="center", fontsize=8)
    ax.set_yticks([]); ax.set_ylabel(alg, rotation=0, ha="right", va="center", fontsize=8)
axes[-1].set_xlabel("Time")
plt.tight_layout(); plt.savefig("gantt_charts.png", dpi=150)
print("Saved comparison_graph.png and gantt_charts.png")
plt.show()
