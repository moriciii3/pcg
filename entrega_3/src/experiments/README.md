# Experimentos

Tres experimentos sobre el generador (Greedy + Hill Climbing con reinicios).
Todos parten de `src/instances/inst1.txt` (curva de 4 puntos) e `inst2.txt` (recta),
cambiando solo los parámetros que indica cada experimento.

## Experimento 1: mejora del Hill Climbing por iteración

**Qué mide:** cuánto mejora el Hill Climbing la solución del Greedy, iteración por iteración
(la iteración 0 es la solución del Greedy). Se usan inst1 e inst2 con las semillas 1 a 10.

**Cómo ejecutarlo:**
```
python exp1_hc_iteraciones/run.py
```
**Resultado:** `exp1_hc_iteraciones/results/exp1_z_vs_iter.png`

## Experimento 2: calidad según la cantidad de reinicios

**Qué mide:** cómo cambia la mejor Z y el tiempo al usar 1, 2, 4, 8, 16 o 32 reinicios.
Se usan inst1 e inst2 con las semillas 1 a 10.

**Cómo ejecutarlo:**
```
python exp2_restarts/run.py
```
**Resultado:** `exp2_restarts/results/exp2_boxplot_z.png` y `exp2_tiempo.png`

## Experimento 3: misma curva, variando L_target

**Qué mide:** qué linealidad se obtiene y cómo se ve la escena cuando se pide
L_target entre 0 y 1. Se usa inst1 con las semillas 1 a 5.

**Cómo ejecutarlo:**
```
python exp3_linealidad/run.py
```
**Resultado:** `exp3_linealidad/results/exp3_L_vs_Ltarget.png` y `exp3_comparativa.png`

## Cómo replicar los resultados

1. Compilar el generador desde `src/algorythm/`:
   ```
   g++ -O2 -std=c++17 level.cpp greedy.cpp hill_climbing.cpp main.cpp -o pcg
   ```
2. Instalar matplotlib: `pip install matplotlib pillow numpy`
3. Desde `src/experiments/`, correr el script de cada experimento (comandos de arriba).

Cada instancia de `instances/` trae su semilla, así que al ejecutarlas se obtienen exactamente
los mismos niveles, CSV y gráficos que los de `results/`. Una ejecución individual se puede
revisar a mano desde `src/algorythm/`:
```
./pcg ../experiments/exp2_restarts/instances/<instancia>.txt
```