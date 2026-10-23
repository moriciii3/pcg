#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// Funcion miope: costo del nivel construido hasta la columna i.
// Compara lo colocado con lo que "deberia" llevar segun el avance.
// costo = alpha * sum_k |o_k - O_k * avance| / sum_k O_k + beta * |L - L_target|
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
// En cada columna se prueban todas las alternativas (patrones de columna):
//   - columna vacia
//   - plataforma en la fila j, sola, con enemigo encima o con obstaculo encima
// Se descartan las que no son factibles (isFeasible hasta la columna i)
// y se elige la de menor costo miope.
// startingPoint = fila de la primera plataforma (se varia en los re-start)
Level greedy(const Params& P, int startingPoint) {
    Level level(P.W, P.H);

    // Columna 0: plataforma libre en el punto inicial (R5, inicio)
    place(level, 0, startingPoint, EMPTY);
    int prevHeight = startingPoint;  // fila de la ultima plataforma pisable (para desempates)

    const int onTopOptions[3] = {EMPTY, ENEMY, OBSTACLE};

    for (int i = 1; i < P.W; i++) {
        double bestCost = 1e18;
        int bestJ = -1;              // -1 = dejar la columna vacia
        int bestOnTop = EMPTY;
        bool found = false;          // se encontro alguna alternativa factible

        // Alternativa 1: columna vacia
        if (isFeasible(level, P, i)) {
            bestCost = myopicCost(level, P, i);
            found = true;
        }

        // Alternativa 2: plataforma en la fila j (hasta H-2 para que quepa algo encima)
        for (int j = 0; j <= P.H - 2; j++) {
            for (int onTop : onTopOptions) {
                place(level, i, j, onTop);                    // probar
                if (isFeasible(level, P, i)) {
                    double cost = myopicCost(level, P, i);

                    // Menor costo. En empate, la mas cercana a la ultima plataforma pisable.
                    bool better = cost < bestCost - 1e-9;
                    bool tie    = abs(cost - bestCost) <= 1e-9 && bestJ != -1 &&
                                  abs(j - prevHeight) < abs(bestJ - prevHeight);
                    if (better || tie) {
                        bestCost  = cost;
                        bestJ     = j;
                        bestOnTop = onTop;
                    }
                    found = true;
                }
                clearColumn(level, i);                        // deshacer
            }
        }

        // No deberia pasar: siempre existe una plataforma libre alcanzable
        if (!found) {
            cerr << "Greedy sin alternativas factibles en la columna " << i << "\n";
            return level;
        }

        // Aplicar la mejor alternativa
        if (bestJ != -1) {
            place(level, i, bestJ, bestOnTop);
            if (bestOnTop != OBSTACLE) prevHeight = bestJ;  // el obstaculo no es pisable
        }
    }
    return level;
}