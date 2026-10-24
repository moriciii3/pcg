#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// Movimiento: cambiar el patron de una columna.
// Se guarda el movimiento y su costo, no el nivel completo:
// copiar miles de niveles por iteracion gastaria mucha memoria.
struct Move {
    double cost;   // f(x) del vecino que genera este movimiento
    int col;       // columna que se cambia
    int row;       // fila de la plataforma (-1 = columna vacia)
    int onTop;     // EMPTY, ENEMY u OBSTACLE
};

// Columna pisable: tiene plataforma y no tiene obstaculo encima
static bool isWalkable(const Level& level, int i) {
    for (int j = 0; j < level.H; j++) {
        if (level.cells[i][j] == PLATFORM)
            return j + 1 >= level.H || level.cells[i][j + 1] != OBSTACLE;
    }
    return false;
}

// Aplica un movimiento: borra la columna y pone el nuevo patron
static void applyMove(Level& level, const Move& m) {
    clearColumn(level, m.col);
    if (m.row != -1) place(level, m.col, m.row, m.onTop);
}

// Hill Climbing con mejor mejora.
// En cada iteracion se arma todo el vecindario factible, se elige el vecino
// de menor f(x) y se avanza a el solo si mejora a la solucion actual.
Level hillClimbing(Level solution, const Params& P) {
    double currentCost = evaluate(solution, P);
    bool improvement = true;

    while (improvement) {
        improvement = false;
        vector<Move> neighborhood;

        for (int i = 0; i < P.W; i++) {
            // Ventana a revisar: de la plataforma pisable anterior a la siguiente.
            // Cambiar la columna i solo afecta los saltos dentro de esta ventana.
            int first = i - 1;
            while (first > 0 && !isWalkable(solution, first)) first--;
            int last = i + 1;
            while (last < P.W - 1 && !isWalkable(solution, last)) last++;
            first = max(first, 0);
            last  = min(last, P.W - 1);

            vector<int> backup = solution.cells[i];

            // Todos los patrones de columna: vacia (row = -1) o plataforma en la fila row
            for (int row = -1; row <= P.H - 2; row++) {
                for (int onTop : {EMPTY, ENEMY, OBSTACLE}) {
                    if (row == -1 && onTop != EMPTY) continue;   // la columna vacia es un solo patron

                    Move m = {0.0, i, row, onTop};
                    applyMove(solution, m);

                    // Se guarda solo si es distinto al actual y factible
                    if (solution.cells[i] != backup && isFeasible(solution, P, last, first)) {
                        m.cost = evaluate(solution, P);
                        neighborhood.push_back(m);
                    }
                    solution.cells[i] = backup;                  // deshacer
                }
            }
        }

        // Mejor vecino: el de menor f(x), porque se minimiza
        if (neighborhood.empty()) break;
        Move best = *min_element(neighborhood.begin(), neighborhood.end(),
                                 [](const Move& a, const Move& b) { return a.cost < b.cost; });

        if (best.cost < currentCost - 1e-12) {
            applyMove(solution, best);
            currentCost = best.cost;
            improvement = true;
        }
    }
    return solution;
}