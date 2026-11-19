#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// Funcion miope: Devuelve lo mismo que la funcion de evaluacion, pero ahora se exige un determinado porcentaje de objetos
// Si va en un 25% del nivel, le solicito el 25% de plataformas de las que se pidieron, y con eso calculo la f(x) de evaluacion.
static double myopicCost(const Level& level, const Params& P, int i) {
    double progress = (double)(i + 1) / P.W;
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k] * progress);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
}

// Una alternativa para la columna actual y su costo miope
struct Option {
    double cost;
    int row;      // fila de la plataforma (-1 = columna vacia)
    int onTop;    // EMPTY, ENEMY u OBSTACLE
};

// Greedy constructivo, de izquierda a derecha.
// En cada columna se prueban todas las alternativas (patrones de columna):
//   - columna vacia
//   - plataforma en la fila j, sola, con enemigo encima o con obstaculo encima
// Se descartan las que no son factibles y se elige siempre la de menor costo miope.
// Si hay empate exacto entre varias, se elige una de ellas al azar (con la semilla).
// startingPoint = fila de la primera plataforma (se varia en los re-start)
Level greedy(const Params& P, int startingPoint, mt19937& rng) {
    Level level(P.W, P.H);

    // Columna 0: plataforma libre en el punto inicial (R5, inicio)
    place(level, 0, startingPoint, EMPTY);
    int prevCol = 0;                 // columna de la ultima plataforma pisable

    const int onTopOptions[3] = {EMPTY, ENEMY, OBSTACLE};

    for (int i = 1; i < P.W; i++) {
        vector<Option> options;      // alternativas factibles de esta columna

        // Alternativa 1: columna vacia.
        // Se revisa desde la ultima plataforma pisable: lo anterior ya fue revisado.
        if (isFeasible(level, P, i, prevCol, true))
            options.push_back({myopicCost(level, P, i), -1, EMPTY});

        // Alternativa 2: plataforma en la fila j (hasta la fila de mas arriba, H-1).
        // En la fila H-1 solo cabe una plataforma libre: no hay espacio para un objeto encima.
        for (int j = 0; j <= P.H - 1; j++) {
            for (int onTop : onTopOptions) {
                if (onTop != EMPTY && j + 1 >= P.H) continue;
                place(level, i, j, onTop);                    // probar
                if (isFeasible(level, P, i, prevCol, true))
                    options.push_back({myopicCost(level, P, i), j, onTop});
                clearColumn(level, i);                        // deshacer
            }
        }

        // No deberia pasar: siempre existe una plataforma libre alcanzable
        if (options.empty()) {
            cerr << "Greedy sin alternativas factibles en la columna " << i << "\n";
            return level;
        }

        // Mejor alternativa: la de menor costo miope.
        // Si varias empatan exactamente, se elige una de ellas al azar.
        double bestCost = options[0].cost;
        for (const Option& o : options) bestCost = min(bestCost, o.cost);
        vector<Option> best;
        for (const Option& o : options)
            if (o.cost <= bestCost + 1e-12) best.push_back(o);
        Option chosen = best[uniform_int_distribution<int>(0, (int)best.size() - 1)(rng)];

        // Aplicar la alternativa elegida
        if (chosen.row != -1) {
            place(level, i, chosen.row, chosen.onTop);
            if (chosen.onTop != OBSTACLE) prevCol = i;        // el obstaculo no es pisable
        }
    }
    return level;
}