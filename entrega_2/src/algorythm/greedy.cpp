#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// Funcion miope: costo del nivel construido hasta la columna i.
// Compara lo colocado con lo que "deberia" llevar segun el avance.
// costo = alpha * sum_k |o_k - O_k * avance| + beta * L
static double myopicCost(const Level& level, const Params& P, int i) {
    double progress = (double)(i + 1) / P.W;
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k] * progress);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
}

// Pone una plataforma en (i, j) y, si se pide, un objeto encima (fila j+1).
// on_top = EMPTY si no va nada encima
static void place(Level& level, int i, int j, int on_top) {
    level.cells[i][j] = PLATFORM;
    if (on_top != EMPTY && j + 1 < level.H) level.cells[i][j + 1] = on_top;
}

// Deja la columna i vacia (sirve para deshacer una prueba)
static void clearColumn(Level& level, int i) {
    for (int j = 0; j < level.H; j++) level.cells[i][j] = EMPTY;
}

// Greedy constructivo, de izquierda a derecha.
// startingPoint = fila de la primera plataforma (se varia en los re-start)
Level greedy(const Params& P, int startingPoint) {
    Level level(P.W, P.H);

    // Columna 0: plataforma en el punto inicial
    place(level, 0, startingPoint, EMPTY);
    int prevHeight = startingPoint; // fila de la ultima plataforma
    int emptyCols = 0; // columnas seguidas sin plataforma

    const int onTopOptions[3] = {EMPTY, ENEMY, OBSTACLE};

    for (int i = 1; i < P.W; i++) {
        double bestCost = 1e18;
        int bestJ = -1;              // -1 = dejar la columna vacia
        int bestOnTop = EMPTY;

        // Opcion 1: columna vacia (solo si no supera el salto maximo en x)
        if (emptyCols + 1 <= P.max_x_jump) {
            bestCost = myopicCost(level, P, i);
        }

        // Opcion 2: plataforma en la fila j, respetando el salto maximo en y
        // Fila 0 = abajo, fila H-1 = arriba. Se llega hasta H-2 para que quepa algo encima (j+1)
        int jMin = max(0, prevHeight - P.max_y_jump);
        int jMax = min(P.H - 2, prevHeight + P.max_y_jump);

        for (int j = jMin; j <= jMax; j++) {
            for (int onTop : onTopOptions) {
                place(level, i, j, onTop);              // probar
                double cost = myopicCost(level, P, i);
                clearColumn(level, i);                  // deshacer

                // Se queda con la de menor costo. En empate, la mas cercana a la anterior.
                bool better = cost < bestCost - 1e-9;
                bool tie    = abs(cost - bestCost) <= 1e-9 && bestJ != -1 &&
                              abs(j - prevHeight) < abs(bestJ - prevHeight);
                if (better || tie) {
                    bestCost  = cost;
                    bestJ     = j;
                    bestOnTop = onTop;
                }
            }
        }

        // Aplicar la mejor opcion
        if (bestJ == -1) {
            emptyCols++;
        } else {
            place(level, i, bestJ, bestOnTop);
            prevHeight = bestJ;
            emptyCols  = 0;
        }
    }
    return level;
}