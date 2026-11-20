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
from PIL import Image, ImageChops  # noqa: E402

BASE = "inst1"
L_TARGETS = sorted({round(k / 10, 2) for k in range(11)} | {0.25, 0.75})
SEEDS = range(1, 6)
COMPARE = [0.0, 0.25, 0.5, 0.75, 1.0]


def tag(lt):
    return f"Lt{int(round(lt * 100)):03d}"          # 0.25 -> Lt025 (sin puntos en el nombre)


def trim(im, pad=20):
    """Recorta el borde blanco de una imagen (format/main.py deja mucho margen vertical)."""
    bbox = ImageChops.difference(im, Image.new("RGB", im.size, "white")).getbbox()
    if not bbox:
        return im
    l, t, r, b = bbox
    return im.crop((max(l - pad, 0), max(t - pad, 0), min(r + pad, im.width), min(b + pad, im.height)))


def main():
    t0 = time.perf_counter()
    D = C.exp_dirs(HERE)
    C.ensure_pcg()

    rows = []
    plot_only = "--plot-only" in sys.argv        # rehace graficos y resumen desde el CSV, sin ejecutar
    if plot_only:
        with open(os.path.join(D["results"], "exp3_linealidad.csv")) as f:
            rows = [{"L_target": float(r["L_target"]), "seed": int(r["seed"]),
                     "L_obtenida": float(r["L_obtenida"]), "Z": float(r["Z"]),
                     "plataformas": int(r["plataformas"]), "enemigos": int(r["enemigos"]),
                     "obstaculos": int(r["obstaculos"])} for r in csv.DictReader(f)]
    for lt in L_TARGETS if not plot_only else []:
        for s in SEEDS:
            name = f"{BASE}_{tag(lt)}_s{s}"
            ipath = C.write_instance(C.BASES[BASE], {"L_target": lt, "seed": s},
                                     os.path.join(D["instances"], name + ".txt"),
                                     comment=f"Exp3: copia de {BASE} con L_target {lt}, seed {s}")
            C.run_pcg(ipath, D["run"])
            h = C.read_header(os.path.join(D["out"], f"{name}_seed{s}_hc.txt"))
            rows.append({"L_target": lt, "seed": s, "L_obtenida": h["linearity"], "Z": h["Z"],
                         "plataformas": int(h["platform"]), "enemigos": int(h["enemy"]),
                         "obstaculos": int(h["obstacle"])})
        sel = [r for r in rows if r["L_target"] == lt]
        print(f"L_target={lt:4.2f}: L prom={sum(r['L_obtenida'] for r in sel) / len(sel):.4f}  "
              f"Z prom={sum(r['Z'] for r in sel) / len(sel):.3e}")

    fields = ["L_target", "seed", "L_obtenida", "Z", "plataformas", "enemigos", "obstaculos"]
    with open(os.path.join(D["results"], "exp3_linealidad.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow({**r, "L_obtenida": f"{r['L_obtenida']:.6g}", "Z": f"{r['Z']:.6g}"})

    resumen = []
    for lt in L_TARGETS:
        sel = [r for r in rows if r["L_target"] == lt]
        lm, lsd = C.mean_std([r["L_obtenida"] for r in sel])
        zm, zsd = C.mean_std([r["Z"] for r in sel])
        resumen.append({"L_target": lt, "L_prom": lm, "L_desv": lsd, "Z_prom": zm, "Z_desv": zsd,
                        "plataformas_prom": sum(r["plataformas"] for r in sel) / len(sel),
                        "enemigos_prom": sum(r["enemigos"] for r in sel) / len(sel),
                        "obstaculos_prom": sum(r["obstaculos"] for r in sel) / len(sel)})
    with open(os.path.join(D["results"], "exp3_resumen.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(resumen[0].keys()))
        w.writeheader()
        for r in resumen:
            w.writerow({k: (f"{v:.6g}" if isinstance(v, float) else v) for k, v in r.items()})

    # L obtenida vs L_target
    fig, ax = plt.subplots(figsize=(6, 5))
    ax.plot([0, 1], [0, 1], linestyle="--", color="#999", label="L obtenida = L_target")
    ax.errorbar([r["L_target"] for r in resumen], [r["L_prom"] for r in resumen],
                yerr=[r["L_desv"] for r in resumen], marker="o", capsize=3, color="#2E6FD8",
                label="L obtenida (promedio ± desv., 5 semillas)")
    ax.set_xlabel("L_target (pedida)")
    ax.set_ylabel("L obtenida")
    ax.set_xlim(-0.03, 1.03)
    ax.set_ylim(-0.03, 1.03)
    ax.set_aspect("equal")
    ax.grid(alpha=0.3)
    ax.legend(fontsize=8, loc="upper left")
    ax.set_title("Experimento 3: linealidad obtenida vs. pedida (inst1)")
    fig.tight_layout()
    fig.savefig(os.path.join(D["results"], "exp3_L_vs_Ltarget.png"), dpi=150)
    plt.close(fig)

    # Escenas: se grafican todas las _hc con format/main.py y se unen las de la semilla 1
    if not plot_only:
        hc_txts = sorted(os.path.join(D["out"], f) for f in os.listdir(D["out"]) if f.endswith("_hc.txt"))
        subprocess.run([sys.executable, C.FORMAT_MAIN, *hc_txts], cwd=os.path.dirname(C.FORMAT_MAIN),
                       check=True, stdout=subprocess.DEVNULL)
    pngs = [os.path.join(D["out"], f"{BASE}_{tag(lt)}_s1_seed1_hc.png") for lt in COMPARE]
    imgs = [trim(Image.open(p).convert("RGB")) for p in pngs]
    width = max(im.width for im in imgs)
    canvas = Image.new("RGB", (width, sum(im.height for im in imgs)), "white")
    y = 0
    for im in imgs:
        canvas.paste(im, (0, y))
        y += im.height
    canvas.save(os.path.join(D["results"], "exp3_comparativa.png"))

    print(f"\n{'L_target':>8} {'L prom':>8} {'L desv':>8} {'Z prom':>10} {'plat':>6} {'enem':>6} {'obst':>6}")
    for r in resumen:
        print(f"{r['L_target']:8.2f} {r['L_prom']:8.4f} {r['L_desv']:8.4f} {r['Z_prom']:10.3e} "
              f"{r['plataformas_prom']:6.1f} {r['enemigos_prom']:6.1f} {r['obstaculos_prom']:6.1f}")
    print(f"\nTiempo total: {time.perf_counter() - t0:.1f} s")


if __name__ == "__main__":
    main()
