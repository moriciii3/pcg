#include "level.h"
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;

// Lee una instancia desde un archivo con lineas "nombre valor".
// Las lineas vacias o que empiezan con # se ignoran.
// Devuelve false si el archivo no existe o falta algun parametro.
bool readInstance(const string& path, Params& P) {
    ifstream f(path);
    if (!f) {
        cerr << "No se pudo abrir la instancia: " << path << "\n";
        return false;
    }

    map<string, double> values;
    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        string key;
        double value;
        if (ss >> key >> value) values[key] = value;
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
    return true;
}

int main(int argc, char* argv[]) {
    string instancePath = (argc > 1) ? argv[1] : "../instances/inst1.txt";

    Params P;
    if (!readInstance(instancePath, P)) return 1;

    // Nombre de la instancia (sin carpeta ni extension), para los archivos de salida
    string instanceName = filesystem::path(instancePath).stem().string();
    cout << "Instancia: " << instanceName << " (" << instancePath << ")\n\n";

    filesystem::create_directories("../out");

    // Reinicios: greedy + Hill Climbing desde cada punto inicial, se guarda el mejor
    Level best(P.W, P.H);
    Level bestGSol(P.W, P.H);
    double bestCost = 1e18;

    for (int start = 0; start <= P.H - 2; start++) {
        Level greedySol = greedy(P, start);
        Level hcSol     = hillClimbing(greedySol, P);

        double zGreedy = evaluate(greedySol, P);
        double zHC     = evaluate(hcSol, P);
        printf("start=%2d   Z greedy=%.5f   Z HC=%.5f   factible=%s\n",
               start, zGreedy, zHC, isFeasible(hcSol, P) ? "si" : "no");

        if (isFeasible(hcSol, P) && zHC < bestCost) {
            bestCost = zHC;
            best = hcSol;
            bestGSol = greedySol; // Almacenamos la solucion greedy correspondiente
        }
    }

    // Mejor solucion
    string path_hc     = "../out/" + instanceName + "_hc.txt";
    string path_greedy = "../out/" + instanceName + "_greedy.txt";
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

    cout << "\nLinealidad  = " << linearity(best) << "  (pedida " << P.L_target << ")\n";
    cout << "Plataformas = " << countObjects(best, PLATFORM) << " / " << P.target_count[PLATFORM] << "\n";
    cout << "Obstaculos  = " << countObjects(best, OBSTACLE) << " / " << P.target_count[OBSTACLE] << "\n";
    cout << "Enemigos    = " << countObjects(best, ENEMY)    << " / " << P.target_count[ENEMY]    << "\n";
    cout << "HC guardado en "     << path_hc     << "\n";
    cout << "Greedy guardado en " << path_greedy << "\n";

    return 0;
}