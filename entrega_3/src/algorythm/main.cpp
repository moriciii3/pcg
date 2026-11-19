#include "level.h"
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;

// Lee una instancia desde un archivo con lineas "nombre valor".
// Las lineas vacias o que empiezan con # se ignoran.
// Los puntos de la curva guia van uno por linea: "x 10 y 4" (x = columna, y = fila).
// Con 2 puntos (o ninguno) la curva es una recta y la linealidad es la de siempre.
// La semilla va como "seed 42" (si no esta, se usa 1).
// Devuelve false si el archivo no existe o falta algun parametro obligatorio.
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

    // Parametros obligatorios
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

    P.target_count[EMPTY]    = 0;     // no se usa
    P.target_count[PLATFORM] = (int)values["platforms"];
    P.target_count[ENEMY]    = (int)values["enemies"];
    P.target_count[OBSTACLE] = (int)values["obstacles"];

    P.alpha    = values["alpha"];
    P.beta     = values["beta"];
    P.L_target = values["L_target"];

    // Parametros opcionales (con valor por defecto)
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

int main(int argc, char* argv[]) {
    string instancePath = (argc > 1) ? argv[1] : "../instances/inst1.txt";

    Params P;
    if (!readInstance(instancePath, P)) return 1;
    int seed = P.seed;
    mt19937 rng(seed);

    // Nombre de la instancia
    string instanceName = filesystem::path(instancePath).stem().string();
    cout << "Instancia: " << instanceName << " (" << instancePath << ")   semilla: " << seed << "\n\n";

    filesystem::create_directories("../out");

    // Reinicios se hacen con greedy + Hill Climbing desde un punto inicial (fila) elegido al azar.
    // Se guarda el mejor
    Level best(P.W, P.H);
    Level bestGSol(P.W, P.H);
    double bestCost = 1e18;

    for (int r = 0; r < P.restarts; r++) {
        int start = uniform_int_distribution<int>(0, P.H - 1)(rng);
        Level greedySol = greedy(P, start, rng);
        Level hcSol     = hillClimbing(greedySol, P);

        double zGreedy = evaluate(greedySol, P);
        double zHC     = evaluate(hcSol, P);
        printf("reinicio=%2d  start=%2d   Z greedy=%.5f   Z HC=%.5f   factible=%s\n",
               r, start, zGreedy, zHC, isFeasible(hcSol, P) ? "si" : "no");

        if (isFeasible(hcSol, P) && zHC < bestCost) {
            bestCost = zHC;
            best = hcSol;
            bestGSol = greedySol; // Almacenamos la solucion greedy correspondiente
        }
    }

    // Mejor solucion
    string base        = "../out/" + instanceName + "_seed" + to_string(seed);
    string path_hc     = base + "_hc.txt";
    string path_greedy = base + "_greedy.txt";
    saveLevel(best, P, path_hc);
    saveLevel(bestGSol, P, path_greedy);

    // Comparacion greedy vs HC de la mejor solucion
    double zGreedyBest = evaluate(bestGSol, P);
    double zHCBest     = bestCost;

    cout << "\n--- Mejor solucion ---\n";
    cout << "Z greedy    = " << zGreedyBest << "\n";
    cout << "Z HC        = " << zHCBest << "\n";
    if (zGreedyBest > 0)
        cout << "Mejora      = " << 100.0 * (zGreedyBest - zHCBest) / zGreedyBest << " %\n";

    cout << "\nLinealidad  = " << linearity(best, P) << "  (pedida " << P.L_target << ")";
    cout << (P.guide.size() >= 2 ? "  respecto a la curva guia\n" : "  respecto a una recta\n");
    cout << "Plataformas = " << countObjects(best, PLATFORM) << " / " << P.target_count[PLATFORM] << "\n";
    cout << "Obstaculos  = " << countObjects(best, OBSTACLE) << " / " << P.target_count[OBSTACLE] << "\n";
    cout << "Enemigos    = " << countObjects(best, ENEMY)    << " / " << P.target_count[ENEMY]    << "\n";
    cout << "HC guardado en "     << path_hc     << "\n";
    cout << "Greedy guardado en " << path_greedy << "\n";

    return 0;
}