"""
Lee los niveles guardados en out/ y los dibuja.

Uso:
    python main.py                            -> dibuja todos los .txt de out/
    python main.py ../out/inst8_seed1_hc.txt  -> dibuja solo ese archivo

Cada imagen se guarda como .png al lado de su .txt.
Si la instancia tiene puntos de interpolacion, se dibuja la curva guia ajustada.
"""
import sys
import glob
import os
import re
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.patches import Patch
from matplotlib.lines import Line2D

# Tipo -> numero (mismo orden que el enum de C++)
TYPES = ["empty", "platform", "obstacle", "enemy"]
COLORS = ["#f4f1ea", "#7a4b2a", "#5a8f3c", "#c0392b"]   # vacio, plataforma, obstaculo, enemigo
LABELS = ["Vacío", "Plataforma", "Obstáculo", "Enemigo"]
CURVE_COLOR = "#2E6FD8"     # curva ajustada (contra la que se mide la linealidad)
POINTS_COLOR = "#E67E22"    # puntos de la instancia y polinomio que pasa por ellos

CELL = re.compile(r"\((\d+),(\d+)\)\s*->\s*(\w+)")


def read_level(path):
    """Devuelve (params, grid). grid[j][i] = numero del tipo en la columna i, fila j."""
    params = {}
    cells = []
    in_cells = False

    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line == "---":
                in_cells = True
                continue

            if not in_cells:
                key, value = line.split(maxsplit=1)
                if key == "guide":
                    # Puntos de la curva guia: "guide x1 y1 x2 y2 ..." -> [(x1, y1), (x2, y2), ...]
                    nums = [float(v) for v in value.split()]
                    params["guide"] = sorted(zip(nums[0::2], nums[1::2]))
                else:
                    params[key] = float(value) if "." in value or "e" in value else int(value)
            else:
                m = CELL.match(line)
                if m:
                    i, j, t = int(m.group(1)), int(m.group(2)), m.group(3)
                    cells.append((i, j, TYPES.index(t)))

    W, H = params["W"], params["H"]
    grid = [[0] * W for _ in range(H)]
    for i, j, t in cells:
        grid[j][i] = t
    return params, grid


def barycentric(guide, x):
    """Polinomio que pasa por todos los puntos de la guia, en forma baricentrica
    (igual que scipy.interpolate.BarycentricInterpolator y que el C++)."""
    num = den = 0.0
    for k, (xk, yk) in enumerate(guide):
        diff = x - xk
        if abs(diff) < 1e-12:                  # justo sobre un punto de la guia
            return yk
        w = 1.0
        for m, (xm, _) in enumerate(guide):
            if m != k:
                w /= (xk - xm)
        num += w / diff * yk
        den += w / diff
    return num / den


def guide_curve(guide, W):
    """Polinomio interpolador evaluado en cada columna (sin ajustar).
    Fuera del tramo de los puntos se mantiene plano, igual que en el C++."""
    x_min, x_max = guide[0][0], guide[-1][0]
    return [barycentric(guide, max(x_min, min(x_max, i))) for i in range(W)]


def fitted_curve(guide, grid, W, H):
    """Curva contra la que se mide la linealidad (igual que linearity en C++).

    - Con 2 o mas puntos: curva guia, subida o bajada por minimos cuadrados (h = a + g).
      Fuera del tramo de los puntos se mantiene plana.
    - Sin puntos: recta de cualquier pendiente ajustada por minimos cuadrados (h = a + b*i).
    """
    has_guide = len(guide) >= 2
    if has_guide:
        g = guide_curve(guide, W)
    else:
        g = [float(i) for i in range(W)]

    # Altura de la plataforma de cada columna
    gs, hs = [], []
    for i in range(W):
        for j in range(H):
            if grid[j][i] == TYPES.index("platform"):
                gs.append(g[i])
                hs.append(j)
                break
    if len(gs) < 2:
        return None

    # Minimos cuadrados
    mg, mh = sum(gs) / len(gs), sum(hs) / len(hs)
    sgg = sum((a - mg) ** 2 for a in gs)
    sgh = sum((a - mg) * (b - mh) for a, b in zip(gs, hs))
    b = 1.0 if has_guide else (sgh / sgg if sgg > 0 else 0.0)
    a = mh - b * mg
    return [a + b * v for v in g]


def plot_level(path):
    params, grid = read_level(path)
    W, H = params["W"], params["H"]
    guide = params.get("guide", [])

    fig, ax = plt.subplots(figsize=(max(8, W / 8), max(3, H / 4)))
    # origin="lower": la fila 0 queda abajo y la fila H-1 arriba
    ax.imshow(grid, cmap=ListedColormap(COLORS), vmin=0, vmax=len(TYPES) - 1,
              origin="lower", interpolation="nearest")

    # Polinomio interpolador original con los puntos de la instancia:
    # muestra que la curva pasa exactamente por cada punto
    if len(guide) >= 2:
        xs_fine = [k / 10 for k in range(int((W - 1) * 10) + 1)]
        x_min, x_max = guide[0][0], guide[-1][0]
        ys_fine = [barycentric(guide, max(x_min, min(x_max, x))) for x in xs_fine]
        ax.plot(xs_fine, ys_fine, color=POINTS_COLOR, linewidth=1.2, linestyle=":")
        ax.plot([x for x, _ in guide], [y for _, y in guide], "o", color=POINTS_COLOR,
                markersize=6, markeredgecolor="white", markeredgewidth=0.8, zorder=5)

    # Curva guia ajustada (o recta ajustada, si la instancia no tiene puntos):
    # es contra la que se mide la linealidad
    curve = fitted_curve(guide, grid, W, H)
    if curve:
        ax.plot(range(W), curve, color=CURVE_COLOR, linewidth=1.5, linestyle="--")

    # Alto visible: el mapa, ampliado si algun punto de la instancia queda fuera
    y_low, y_high = -0.5, H - 0.5
    if guide:
        y_low = min(y_low, min(y for _, y in guide) - 0.5)
        y_high = max(y_high, max(y for _, y in guide) + 0.5)
    ax.set_ylim(y_low, y_high)

    # Grilla suave entre casillas
    ax.set_xticks([x - 0.5 for x in range(W + 1)], minor=True)
    ax.set_yticks([y - 0.5 for y in range(H + 1)], minor=True)
    ax.grid(which="minor", color="white", linewidth=0.3)
    ax.tick_params(which="minor", length=0)
    ax.set_xlabel("i (columna)")
    ax.set_ylabel("j (fila)")

    name = os.path.splitext(os.path.basename(path))[0]
    title = (f"{name}\n"
             f"W={W}  H={H}  max_x_jump={params['max_x_jump']}  max_y_jump={params['max_y_jump']}  "
             f"α={params['alpha']}  β={params['beta']}  L_target={params.get('L_target')}  "
             f"puntos guía={len(guide)}  semilla={params.get('seed')}\n"
             f"Z={params['Z']:.5f}   L={params['linearity']:.3f}   "
             f"plataformas {params['platform']}/{params['target_platform']}   "
             f"obstáculos {params['obstacle']}/{params['target_obstacle']}   "
             f"enemigos {params['enemy']}/{params['target_enemy']}")
    ax.set_title(title, fontsize=9)

    legend = [Patch(color=c, label=l) for c, l in zip(COLORS, LABELS)]
    if len(guide) >= 2:
        legend.append(Line2D([0], [0], color=POINTS_COLOR, linewidth=1.2, linestyle=":", marker="o",
                             markersize=5, label="Puntos y polinomio"))
    if curve:
        label = "Curva guía ajustada" if len(guide) >= 2 else "Recta ajustada"
        legend.append(Line2D([0], [0], color=CURVE_COLOR, linewidth=1.5, linestyle="--", label=label))
    ax.legend(handles=legend, loc="upper left", bbox_to_anchor=(1.0, 1.0), fontsize=8)

    out = path.rsplit(".", 1)[0] + ".png"
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    plt.close(fig)
    print(f"{path} -> {out}")


if __name__ == "__main__":
    files = sys.argv[1:] or sorted(glob.glob("../out/*.txt"))
    if not files:
        print("No hay archivos .txt en out/")
    for path in files:
        plot_level(path)