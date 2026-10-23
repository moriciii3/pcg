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

    // Funcion objetivo: Z = alpha * D + beta * T   (alpha + beta = 1)
    P.alpha = 0.7;                // 70% importan las cantidades
    P.beta  = 0.3;                // 30% importa la forma del nivel
    P.L_target = 0.85;            // alta linealidad (1 = recta, 0.5 = sube y baja mucho)

    // Crear la carpeta out/ si no existe
    filesystem::create_directories("out");

    // Probar el greedy con un punto inicial
    int start = 5;
    Level level = greedy(P, start);

    string path = "../out/greedy_alta_linealidad.txt";
    saveLevel(level, P, path);

    cout << "Factible    = " << (isFeasible(level, P) ? "si" : "no") << "\n";
    cout << "Z           = " << evaluate(level, P) << "\n";
    cout << "Linealidad  = " << linearity(level) << "\n";
    cout << "Plataformas = " << countObjects(level, PLATFORM) << " / " << P.target_count[PLATFORM] << "\n";
    cout << "Obstaculos  = " << countObjects(level, OBSTACLE) << " / " << P.target_count[OBSTACLE] << "\n";
    cout << "Enemigos    = " << countObjects(level, ENEMY)    << " / " << P.target_count[ENEMY]    << "\n";
    cout << "Guardado en " << path << "\n";

    return 0;
}