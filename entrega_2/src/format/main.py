"""
Lee los niveles guardados en out/ y los dibuja.

Uso:
    python visualize.py                      -> dibuja todos los .txt de out/
    python visualize.py out/greedy_start5.txt -> dibuja solo ese archivo

Cada imagen se guarda como .png al lado de su .txt.
"""
import sys
import glob
import re
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.patches import Patch

# Tipo -> numero (mismo orden que el enum de C++)
TYPES = ["empty", "platform", "obstacle", "enemy"]
COLORS = ["#f4f1ea", "#7a4b2a", "#5a8f3c", "#c0392b"]   # vacio, plataforma, obstaculo, enemigo
LABELS = ["Vacío", "Plataforma", "Obstáculo", "Enemigo"]

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


def plot_level(path):
    params, grid = read_level(path)
    W, H = params["W"], params["H"]

    fig, ax = plt.subplots(figsize=(max(8, W / 8), max(3, H / 4)))
    # origin="lower": la fila 0 queda abajo y la fila H-1 arriba
    ax.imshow(grid, cmap=ListedColormap(COLORS), vmin=0, vmax=len(TYPES) - 1,
              origin="lower", interpolation="nearest")

    # Grilla suave entre casillas
    ax.set_xticks([x - 0.5 for x in range(W + 1)], minor=True)
    ax.set_yticks([y - 0.5 for y in range(H + 1)], minor=True)
    ax.grid(which="minor", color="white", linewidth=0.3)
    ax.tick_params(which="minor", length=0)
    ax.set_xlabel("i (columna)")
    ax.set_ylabel("j (fila)")

    title = (f"W={W}  H={H}  max_x_jump={params['max_x_jump']}  max_y_jump={params['max_y_jump']}  "
             f"α={params['alpha']}  β={params['beta']}\n"
             f"Z={params['Z']:.3f}   L={params['linearity']:.3f}   "
             f"plataformas {params['platform']}/{params['target_platform']}   "
             f"obstáculos {params['obstacle']}/{params['target_obstacle']}   "
             f"enemigos {params['enemy']}/{params['target_enemy']}")
    ax.set_title(title, fontsize=9)

    legend = [Patch(color=c, label=l) for c, l in zip(COLORS, LABELS)]
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