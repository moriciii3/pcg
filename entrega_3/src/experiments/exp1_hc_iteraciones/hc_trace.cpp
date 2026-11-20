// Experimento 1: Z del Hill Climbing iteracion por iteracion.
//
// No modifica el codigo fuente: incluye level.h y se compila junto a
// level.cpp, greedy.cpp y hill_climbing.cpp originales (sin main.cpp):
//
//   g++ -O2 -std=c++17 hc_trace.cpp ../../algorythm/level.cpp ../../algorythm/greedy.cpp \
//       ../../algorythm/hill_climbing.cpp -o ../bin/hc_trace
//
// Uso: hc_trace <instancia> <semilla>
//
// Hace lo mismo que el reinicio 0 de pcg con esa semilla:
//   rng(seed), start = uniform(0, H-1)(rng), greedy(P, start, rng)
// Luego corre hillClimbingTrace (copia de hillClimbing que registra Z en cada iteracion)
// y, para verificar la copia, el hillClimbing original sobre la misma solucion greedy.
//
// Salida (stdout):
//   iter,Z            <- una linea por iteracion (iteracion 0 = solucion greedy)
//   # start=.. iters=.. Z_greedy=.. Z_trace=.. Z_orig=.. check=OK|MISMATCH
#include "../../algorythm/level.h"
#include <bits/stdc++.h>
using namespace std;

// ---------------------------------------------------------------------------
// Copia de readInstance (main.cpp)
// ---------------------------------------------------------------------------
bool readInstance(const string& path, Params& P) {
    ifstream f(path);
    if (!f) {
        cerr << "No se pudo abrir la instancia: " << path << "\n";
        return false;
    }

    map<string, double> values;
    P.guide.clear();
    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        string key;
        if (!(ss >> key)) continue;

        if (key == "x") {
            double x, y;
            string yKey;
            if (ss >> x >> yKey >> y && yKey == "y") P.guide.push_back({x, y});
            else cerr << "Punto mal escrito (se esperaba \"x <columna> y <fila>\"): " << line << "\n";
            continue;
        }
        double value;
        if (ss >> value) values[key] = value;
    }

    vector<string> required = {"W", "H", "max_x_jump", "max_y_jump",
                               "platforms", "enemies", "obstacles",
                               "alpha", "beta", "L_target"};
    for (const string& key : required) {
        if (!values.count(key)) {
            cerr << "Falta el parametro '" << key << "' en " << path << "\n";
            return false;
        }
    }

    P.W          = (int)values["W"];
    P.H          = (int)values["H"];
    P.max_x_jump = (int)values["max_x_jump"];
    P.max_y_jump = (int)values["max_y_jump"];

    P.target_count[EMPTY]    = 0;
    P.target_count[PLATFORM] = (int)values["platforms"];
    P.target_count[ENEMY]    = (int)values["enemies"];
    P.target_count[OBSTACLE] = (int)values["obstacles"];

    P.alpha    = values["alpha"];
    P.beta     = values["beta"];
    P.L_target = values["L_target"];

    P.restarts   = values.count("restarts")   ? (int)values["restarts"]   : 16;
    P.seed       = values.count("seed")       ? (int)values["seed"]       : 1;

    sort(P.guide.begin(), P.guide.end());
    for (size_t k = 1; k < P.guide.size(); k++) {
        if (P.guide[k].first == P.guide[k - 1].first) {
            cerr << "La curva guia tiene dos puntos con la misma columna x = " << P.guide[k].first << "\n";
            return false;
        }
    }
    if (P.guide.size() >= 2 && (P.guide.front().first > 0 || P.guide.back().first < P.W - 1))
        cerr << "Aviso: los puntos no cubren todo el nivel (columnas 0 a " << P.W - 1
             << "); fuera de ellos la curva se mantiene plana.\n";
    prepareShape(P);
    return true;
}

// ---------------------------------------------------------------------------
// Copia de hill_climbing.cpp (Move, isWalkable, applyMove son static alla,
// asi que no chocan con estas copias)
// ---------------------------------------------------------------------------
struct Move {
    double cost;
    int col;
    int row;
    int onTop;
};

static bool isWalkable(const Level& level, int i) {
    for (int j = 0; j < level.H; j++) {
        if (level.cells[i][j] == PLATFORM)
            return j + 1 >= level.H || level.cells[i][j + 1] != OBSTACLE;
    }
    return false;
}

static void applyMove(Level& level, const Move& m) {
    clearColumn(level, m.col);
    if (m.row != -1) place(level, m.col, m.row, m.onTop);
}

// Igual a hillClimbing, pero guarda en `trace` el Z de la solucion actual:
// trace[0] = solucion inicial (greedy), trace[t] = despues de la t-esima mejora aceptada.
Level hillClimbingTrace(Level solution, const Params& P, vector<double>& trace) {
    double currentCost = evaluate(solution, P);
    trace.push_back(currentCost);                                  // <- agregado
    bool improvement = true;

    while (improvement) {
        improvement = false;
        vector<Move> neighborhood;

        for (int i = 0; i < P.W; i++) {
            int first = i - 1;
            while (first > 0 && !isWalkable(solution, first)) first--;
            int last = i + 1;
            while (last < P.W - 1 && !isWalkable(solution, last)) last++;
            first = max(first, 0);
            last  = min(last, P.W - 1);

            vector<int> backup = solution.cells[i];

            for (int row = -1; row <= P.H - 2; row++) {
                for (int onTop : {EMPTY, ENEMY, OBSTACLE}) {
                    if (row == -1 && onTop != EMPTY) continue;

                    Move m = {0.0, i, row, onTop};
                    applyMove(solution, m);

                    if (solution.cells[i] != backup && isFeasible(solution, P, last, first)) {
                        m.cost = evaluate(solution, P);
                        neighborhood.push_back(m);
                    }
                    solution.cells[i] = backup;
                }
            }
        }

        if (neighborhood.empty()) break;
        Move best = *min_element(neighborhood.begin(), neighborhood.end(),
                                 [](const Move& a, const Move& b) { return a.cost < b.cost; });

        if (best.cost < currentCost - 1e-12) {
            applyMove(solution, best);
            currentCost = best.cost;
            improvement = true;
            trace.push_back(currentCost);                          // <- agregado
        }
    }
    return solution;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: hc_trace <instancia> <semilla>\n";
        return 1;
    }
    Params P;
    if (!readInstance(argv[1], P)) return 1;
    int seed = atoi(argv[2]);
    P.seed = seed;

    // Igual que el reinicio 0 de pcg
    mt19937 rng(seed);
    int start = uniform_int_distribution<int>(0, P.H - 1)(rng);
    Level greedySol = greedy(P, start, rng);

    vector<double> trace;
    Level hcTrace = hillClimbingTrace(greedySol, P, trace);
    Level hcOrig  = hillClimbing(greedySol, P);                    // original, para verificar

    double zG = evaluate(greedySol, P);
    double zT = evaluate(hcTrace, P);
    double zO = evaluate(hcOrig, P);
    bool ok = fabs(zT - zO) < 1e-12 && hcTrace.cells == hcOrig.cells && fabs(trace.back() - zT) < 1e-12;

    printf("iter,Z\n");
    for (size_t t = 0; t < trace.size(); t++) printf("%zu,%.10g\n", t, trace[t]);
    printf("# start=%d iters=%zu Z_greedy=%.10g Z_trace=%.10g Z_orig=%.10g feasible=%s check=%s\n",
           start, trace.size() - 1, zG, zT, zO, isFeasible(hcTrace, P) ? "si" : "no",
           ok ? "OK" : "MISMATCH");
    return ok ? 0 : 2;
}
