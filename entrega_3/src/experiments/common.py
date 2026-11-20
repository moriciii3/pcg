"""Utilidades compartidas por los tres experimentos.

- Rutas del proyecto (instancias base, pcg compilado, format/main.py).
- write_instance: copia una instancia base cambiando solo algunas claves.
- run_pcg: ejecuta pcg con el directorio de trabajo en expN/run/ (asi escribe en expN/out/).
- parse_console / read_header: leen la salida de consola y el encabezado de los .txt.
"""
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))           # src/experiments
SRC = os.path.dirname(HERE)                                   # src
ALGO = os.path.join(SRC, "algorythm")
BASE_INSTANCES = os.path.join(SRC, "instances")
FORMAT_MAIN = os.path.join(SRC, "format", "main.py")
BIN = os.path.join(HERE, "bin")
PCG = os.path.join(BIN, "pcg")

# Fuentes del programa (sin modificar)
SOURCES = [os.path.join(ALGO, f) for f in ("level.cpp", "greedy.cpp", "hill_climbing.cpp", "main.cpp")]

# inst1 = curva de 4 puntos, inst2 = recta de 2 puntos
BASES = {"inst1": os.path.join(BASE_INSTANCES, "inst1.txt"),
         "inst2": os.path.join(BASE_INSTANCES, "inst2.txt")}


def ensure_pcg():
    """Compila pcg desde las fuentes originales en experiments/bin/ si falta o esta desactualizado."""
    os.makedirs(BIN, exist_ok=True)
    newest = max(os.path.getmtime(p) for p in SOURCES + [os.path.join(ALGO, "level.h")])
    if not os.path.exists(PCG) or os.path.getmtime(PCG) < newest:
        cmd = ["g++", "-O2", "-std=c++17", *SOURCES, "-o", PCG]
        print("Compilando:", " ".join(cmd))
        subprocess.run(cmd, check=True)
    return PCG


def exp_dirs(exp_dir):
    """Crea instances/, out/, results/ y run/ dentro del experimento y devuelve sus rutas."""
    d = {k: os.path.join(exp_dir, k) for k in ("instances", "out", "results", "run")}
    for p in d.values():
        os.makedirs(p, exist_ok=True)
    return d


def write_instance(base_path, overrides, out_path, comment=None):
    """Copia la instancia base cambiando solo las claves de `overrides` (p. ej. {'seed': 3})."""
    done = set()
    lines = []
    if comment:
        lines.append(f"# {comment}\n")
    with open(base_path) as f:
        for line in f:
            parts = line.split()
            if parts and not line.startswith("#") and parts[0] in overrides:
                key = parts[0]
                lines.append(f"{key} {overrides[key]}\n")
                done.add(key)
            else:
                lines.append(line if line.endswith("\n") else line + "\n")
    for key, value in overrides.items():          # claves que no estaban en la base
        if key not in done:
            lines.append(f"{key} {value}\n")
    with open(out_path, "w") as f:
        f.writelines(lines)
    return out_path


def run_pcg(instance_path, run_dir):
    """Ejecuta pcg (cwd = run_dir, por lo que guarda en run_dir/../out). Devuelve (stdout, segundos)."""
    t0 = time.perf_counter()
    res = subprocess.run([PCG, os.path.abspath(instance_path)], cwd=run_dir,
                         capture_output=True, text=True)
    secs = time.perf_counter() - t0
    if res.returncode != 0:
        sys.exit(f"pcg fallo con {instance_path}:\n{res.stderr}")
    return res.stdout, secs


_RESTART = re.compile(r"reinicio=\s*(\d+)\s+start=\s*(\d+)\s+Z greedy=([\d.eE+-]+)\s+Z HC=([\d.eE+-]+)\s+factible=(\w+)")


def parse_console(stdout):
    """Devuelve dict con 'restarts' (lista de reinicios) y los valores finales (Z_greedy, Z_HC, ...)."""
    out = {"restarts": []}
    for line in stdout.splitlines():
        m = _RESTART.search(line)
        if m:
            out["restarts"].append({"r": int(m.group(1)), "start": int(m.group(2)),
                                    "Z_greedy": float(m.group(3)), "Z_HC": float(m.group(4)),
                                    "feasible": m.group(5) == "si"})
            continue
        if "=" in line and not line.startswith("reinicio"):
            key, _, value = line.partition("=")
            key = key.strip()
            tok = value.split()
            if key in ("Z greedy", "Z HC", "Linealidad") and tok:
                out[key.replace(" ", "_")] = float(tok[0])
    return out


def read_header(path):
    """Lee el encabezado 'clave valor' de un .txt de salida (hasta '---')."""
    params = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line == "---":
                break
            if not line:
                continue
            key, value = line.split(maxsplit=1)
            if key == "guide":
                params[key] = [float(v) for v in value.split()]
            else:
                params[key] = float(value)
    return params


def mean_std(values):
    n = len(values)
    m = sum(values) / n
    sd = (sum((v - m) ** 2 for v in values) / (n - 1)) ** 0.5 if n > 1 else 0.0
    return m, sd
