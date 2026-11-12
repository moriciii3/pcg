#include "level.h"
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;

int main() {
    // Definicion de parametros
    Params P;
    P.max_x_jump = 5;
    P.max_y_jump = 1;
    P.W = 100;
    P.H = 16;

    P.target_count[EMPTY]    = 0;     // no se usa
    P.target_count[PLATFORM] = 75;
    P.target_count[ENEMY]    = 15;
    P.target_count[OBSTACLE] = 35;

    // Funcion objetivo: Z = alpha * D + beta * |L - L_target|   (alpha + beta = 1)
    P.alpha = 0.7;                // 70% importan las cantidades
    P.beta  = 0.3;                // 30% importa la linealidad
    P.L_target = 0.5;            // linealidad buscada (1 = recta, 0.5 = sube y baja mucho)

    filesystem::create_directories("out");

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
    string path_hc = "../out/instancia5_hc.txt";
    string path_greedy = "../out/instancia5_greedy.txt";
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