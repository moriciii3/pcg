#include "level.h"
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;

int main() {
    // Definicion de parametros
    Params P;
    P.max_x_jump = 6;
    P.max_y_jump = 4;
    P.W = 150;
    P.H = 16;

    P.target_count[EMPTY]    = 0;     // no se usa
    P.target_count[PLATFORM] = 100;
    P.target_count[OBSTACLE] = 10;
    P.target_count[ENEMY]    = 15;

    // Funcion objetivo: Z = alpha * D + beta * |L - L_target|   (alpha + beta = 1)
    P.alpha = 0.7;                // 70% importan las cantidades
    P.beta  = 0.3;                // 30% importa la linealidad
    P.L_target = 0.67;            // linealidad buscada (1 = recta, 0.5 = sube y baja mucho)

    filesystem::create_directories("out");

    // Reinicios: greedy + Hill Climbing desde cada punto inicial, se guarda el mejor
    Level best(P.W, P.H);
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
        }
    }

    // Mejor solucion
    string path = "../out/mejor_solucion.txt";
    saveLevel(best, P, path);

    cout << "\nMejor Z     = " << bestCost << "\n";
    cout << "Linealidad  = " << linearity(best) << "\n";
    cout << "Plataformas = " << countObjects(best, PLATFORM) << " / " << P.target_count[PLATFORM] << "\n";
    cout << "Obstaculos  = " << countObjects(best, OBSTACLE) << " / " << P.target_count[OBSTACLE] << "\n";
    cout << "Enemigos    = " << countObjects(best, ENEMY)    << " / " << P.target_count[ENEMY]    << "\n";
    cout << "Guardado en " << path << "\n";

    return 0;
}