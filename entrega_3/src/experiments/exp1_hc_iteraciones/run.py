import csv
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import common as C  # noqa: E402

import matplotlib  # noqa: E402
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import MaxNLocator  # noqa: E402

SEEDS = range(1, 11)
INSTANCES = ["inst1", "inst2"]
TRACE_BIN = os.path.join(C.BIN, "hc_trace")


def build_trace():
    src = [os.path.join(HERE, "hc_trace.cpp")] + [os.path.join(C.ALGO, f) for f in
                                                  ("level.cpp", "greedy.cpp", "hill_climbing.cpp")]
    cmd = ["g++", "-O2", "-std=c++17", *src, "-o", TRACE_BIN]
    print("Compilando:", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    t0 = time.perf_counter()
    D = C.exp_dirs(HERE)
    C.ensure_pcg()
    build_trace()

    trace_rows, summary_rows = [], []
    problems = []
    for inst in INSTANCES:
        for s in SEEDS:
            # Instancia con esa semilla y 1 reinicio: pcg hace exactamente el mismo greedy + HC
            ipath = C.write_instance(C.BASES[inst], {"seed": s, "restarts": 1},
                                     os.path.join(D["instances"], f"{inst}_s{s}.txt"),
                                     comment=f"Exp1: copia de {inst} con seed {s}, restarts 1")

            res = subprocess.run([TRACE_BIN, ipath, str(s)], capture_output=True, text=True)
            lines = res.stdout.strip().splitlines()
            info = dict(kv.split("=") for kv in lines[-1].lstrip("# ").split())
            if info["check"] != "OK":
                problems.append(f"{inst} seed {s}: la copia no coincide con hillClimbing ({lines[-1]})")
            trace = [float(l.split(",")[1]) for l in lines[1:-1]]
            for t, z in enumerate(trace):
                trace_rows.append([inst, s, t, f"{z:.10g}"])

            # Ejecucion de pcg original (guarda el .txt en out/) y verificacion cruzada
            out, _ = C.run_pcg(ipath, D["run"])
            r0 = C.parse_console(out)["restarts"][0]
            if (r0["start"] != int(info["start"]) or abs(r0["Z_greedy"] - trace[0]) > 6e-6
                    or abs(r0["Z_HC"] - trace[-1]) > 6e-6):
                problems.append(f"{inst} seed {s}: pcg reinicio 0 = {r0} no coincide con la traza")

            zg, zh = trace[0], trace[-1]
            summary_rows.append({"instancia": inst, "semilla": s, "start": int(info["start"]),
                                 "Z_greedy": zg, "Z_HC": zh, "mejora_abs": zg - zh,
                                 "mejora_pct": 100 * (zg - zh) / zg if zg > 0 else 0.0,
                                 "iteraciones": len(trace) - 1})
            print(f"{inst} seed {s:2d}: start={info['start']:>2}  Z greedy={zg:.6f}  Z HC={zh:.6f}  "
                  f"iters={len(trace) - 1:3d}  check={info['check']}")

    if problems:
        print("\n*** PROBLEMAS ***")
        print("\n".join(problems))
        sys.exit(1)

    # CSVs
    with open(os.path.join(D["results"], "exp1_trace.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["instancia", "semilla", "iteracion", "Z"])
        w.writerows(trace_rows)
    with open(os.path.join(D["results"], "exp1_por_semilla.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(summary_rows[0].keys()))
        w.writeheader()
        w.writerows(summary_rows)

    metrics = ["Z_greedy", "Z_HC", "mejora_abs", "mejora_pct", "iteraciones"]
    resumen = []
    for inst in INSTANCES:
        rows = [r for r in summary_rows if r["instancia"] == inst]
        row = {"instancia": inst, "n_semillas": len(rows)}
        for m in metrics:
            mu, sd = C.mean_std([r[m] for r in rows])
            row[f"{m}_prom"], row[f"{m}_desv"] = mu, sd
        resumen.append(row)
    with open(os.path.join(D["results"], "exp1_resumen.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(resumen[0].keys()))
        w.writeheader()
        for r in resumen:
            w.writerow({k: (f"{v:.6g}" if isinstance(v, float) else v) for k, v in r.items()})

    # Grafico Z vs iteracion
    fig, axes = plt.subplots(1, 2, figsize=(12, 4.5), sharey=False)
    cmap = plt.get_cmap("tab10")
    titles = {"inst1": "inst1 (curva de 4 puntos)", "inst2": "inst2 (recta de 2 puntos)"}
    for ax, inst in zip(axes, INSTANCES):
        for k, s in enumerate(SEEDS):
            zs = [float(r[3]) for r in trace_rows if r[0] == inst and r[1] == s]
            ax.plot(range(len(zs)), zs, marker="o", markersize=3, linewidth=1.2,
                    color=cmap(k % 10), label=f"semilla {s}")
        ax.set_title(titles[inst])
        ax.set_xlabel("Iteración del Hill Climbing (0 = Greedy)")
        ax.set_ylabel("Z")
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        ax.grid(alpha=0.3)
    axes[1].legend(fontsize=8, loc="upper left", bbox_to_anchor=(1.0, 1.0))
    fig.suptitle("Experimento 1: Z de la solución actual en cada iteración del Hill Climbing")
    fig.tight_layout()
    fig.savefig(os.path.join(D["results"], "exp1_z_vs_iter.png"), dpi=150)
    plt.close(fig)

    # Grafico de los .txt guardados por pcg
    txts = sorted(os.path.join(D["out"], f) for f in os.listdir(D["out"]) if f.endswith(".txt"))
    subprocess.run([sys.executable, C.FORMAT_MAIN, *txts], cwd=os.path.dirname(C.FORMAT_MAIN),
                   check=True, stdout=subprocess.DEVNULL)

    print("\nResumen (promedio ± desviación sobre 10 semillas)")
    print(f"{'inst':6} {'Z greedy':>22} {'Z HC':>22} {'mejora abs':>22} {'mejora %':>16} {'iters':>12}")
    for r in resumen:
        print(f"{r['instancia']:6} "
              + " ".join(f"{r[m + '_prom']:.3e} ± {r[m + '_desv']:.1e}".rjust(22) for m in metrics[:3])
              + f" {r['mejora_pct_prom']:7.2f} ± {r['mejora_pct_desv']:5.2f}"
              + f" {r['iteraciones_prom']:5.1f} ± {r['iteraciones_desv']:4.1f}")
    print(f"\nTiempo total: {time.perf_counter() - t0:.1f} s")


if __name__ == "__main__":
    main()
