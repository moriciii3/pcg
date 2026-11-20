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

RESTARTS = [1, 2, 4, 8, 16, 32]
SEEDS = range(1, 11)
INSTANCES = ["inst1", "inst2"]
TITLES = {"inst1": "inst1 (curva de 4 puntos)", "inst2": "inst2 (recta de 2 puntos)"}


def main():
    t0 = time.perf_counter()
    D = C.exp_dirs(HERE)
    C.ensure_pcg()

    rows = []
    plot_only = "--plot-only" in sys.argv        # rehace graficos y resumen desde el CSV, sin ejecutar
    if plot_only:
        with open(os.path.join(D["results"], "exp2_restarts.csv")) as f:
            rows = [{"instancia": r["instancia"], "restarts": int(r["restarts"]), "seed": int(r["seed"]),
                     "Z_mejor": float(r["Z_mejor"]), "tiempo_s": float(r["tiempo_s"])}
                    for r in csv.DictReader(f)]
    for inst in INSTANCES if not plot_only else []:
        for R in RESTARTS:
            for s in SEEDS:
                ipath = C.write_instance(C.BASES[inst], {"seed": s, "restarts": R},
                                         os.path.join(D["instances"], f"{inst}_r{R}_s{s}.txt"),
                                         comment=f"Exp2: copia de {inst} con seed {s}, restarts {R}")
                out, secs = C.run_pcg(ipath, D["run"])
                z = C.parse_console(out)["Z_HC"]
                rows.append({"instancia": inst, "restarts": R, "seed": s, "Z_mejor": z, "tiempo_s": secs})
            zs = [r["Z_mejor"] for r in rows if r["instancia"] == inst and r["restarts"] == R]
            ts = [r["tiempo_s"] for r in rows if r["instancia"] == inst and r["restarts"] == R]
            print(f"{inst} restarts={R:2d}: Z prom={sum(zs)/len(zs):.3e}  t prom={sum(ts)/len(ts):.2f} s")

    with open(os.path.join(D["results"], "exp2_restarts.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["instancia", "restarts", "seed", "Z_mejor", "tiempo_s"])
        w.writeheader()
        for r in rows:
            w.writerow({**r, "Z_mejor": f"{r['Z_mejor']:.6g}", "tiempo_s": f"{r['tiempo_s']:.4f}"})

    # Resumen por (instancia, restarts)
    resumen = []
    for inst in INSTANCES:
        for R in RESTARTS:
            sel = [r for r in rows if r["instancia"] == inst and r["restarts"] == R]
            zm, zsd = C.mean_std([r["Z_mejor"] for r in sel])
            tm, tsd = C.mean_std([r["tiempo_s"] for r in sel])
            zsorted = sorted(r["Z_mejor"] for r in sel)
            med = (zsorted[4] + zsorted[5]) / 2 if len(zsorted) == 10 else zsorted[len(zsorted) // 2]
            resumen.append({"instancia": inst, "restarts": R, "Z_prom": zm, "Z_desv": zsd, "Z_mediana": med,
                            "Z_min": zsorted[0], "Z_max": zsorted[-1], "t_prom_s": tm, "t_desv_s": tsd})
    with open(os.path.join(D["results"], "exp2_resumen.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(resumen[0].keys()))
        w.writeheader()
        for r in resumen:
            w.writerow({k: (f"{v:.6g}" if isinstance(v, float) else v) for k, v in r.items()})

    # Boxplot de Z vs restarts
    fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))
    for ax, inst in zip(axes, INSTANCES):
        data = [[r["Z_mejor"] for r in rows if r["instancia"] == inst and r["restarts"] == R] for R in RESTARTS]
        ax.boxplot(data, tick_labels=[str(R) for R in RESTARTS], showmeans=True,
                   meanprops={"marker": "^", "markerfacecolor": "#2E6FD8", "markeredgecolor": "#2E6FD8"})
        for k, d in enumerate(data):  # puntos individuales
            ax.scatter([k + 1] * len(d), d, s=10, color="#555", alpha=0.5, zorder=3)
        ax.set_title(TITLES[inst])
        ax.set_xlabel("Reinicios (restarts)")
        ax.set_ylabel("Mejor Z (10 semillas, escala log)")
        ax.set_yscale("log")
        ax.grid(alpha=0.3, axis="y", which="both")
    fig.suptitle("Experimento 2: mejor Z según la cantidad de reinicios (▲ = promedio)")
    fig.tight_layout()
    fig.savefig(os.path.join(D["results"], "exp2_boxplot_z.png"), dpi=150)
    plt.close(fig)

    # Tiempo promedio vs restarts
    fig, ax = plt.subplots(figsize=(6.5, 4.5))
    for inst, color in zip(INSTANCES, ["#2E6FD8", "#E67E22"]):
        sel = [r for r in resumen if r["instancia"] == inst]
        ax.errorbar(RESTARTS, [r["t_prom_s"] for r in sel], yerr=[r["t_desv_s"] for r in sel],
                    marker="o", capsize=3, color=color, label=TITLES[inst])
    ax.set_xscale("log", base=2)
    ax.set_xticks(RESTARTS, [str(R) for R in RESTARTS])
    ax.set_xlabel("Reinicios (restarts)")
    ax.set_ylabel("Tiempo promedio por ejecución [s]")
    ax.set_title("Experimento 2: tiempo vs. reinicios (± desviación)")
    ax.grid(alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(os.path.join(D["results"], "exp2_tiempo.png"), dpi=150)
    plt.close(fig)

    # Escenas de las mejores soluciones (.png al lado de cada .txt en out/)
    if not plot_only:
        txts = sorted(os.path.join(D["out"], f) for f in os.listdir(D["out"]) if f.endswith("_hc.txt"))
        subprocess.run([sys.executable, C.FORMAT_MAIN, *txts], cwd=os.path.dirname(C.FORMAT_MAIN),
                       check=True, stdout=subprocess.DEVNULL)

    print(f"\n{'inst':6} {'R':>3} {'Z prom':>10} {'Z desv':>10} {'Z mediana':>10} {'t prom [s]':>11}")
    for r in resumen:
        print(f"{r['instancia']:6} {r['restarts']:3d} {r['Z_prom']:10.3e} {r['Z_desv']:10.3e} "
              f"{r['Z_mediana']:10.3e} {r['t_prom_s']:11.3f}")
    print(f"\nTiempo total: {time.perf_counter() - t0:.1f} s")


if __name__ == "__main__":
    main()
