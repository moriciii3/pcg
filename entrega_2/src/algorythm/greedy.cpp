#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// Funcion miope: Devuelve lo mismo que la funcion de evaluacion, pero ahora se exige un determinado porcentaje de objetos
// Si va en un 25% del nivel, le solicito el 25% de plataformas de las que se pidieron, y con eso calculo la f(x) de evaluaciónS.
static double myopicCost(const Level& level, const Params& P, int i) {
    double progress = (double)(i + 1) / P.W;
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k] * progress);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
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