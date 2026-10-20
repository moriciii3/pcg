#include "level.h"
#include <bits/stdc++.h>
using namespace std;

int main() {
    // Definicion de parametros
    Params P;
    P.max_x_jump = 6;
    P.max_y_jump = 4;
    P.W = 150;
    P.H = 16;
 
    P.target_count[EMPTY]    = 0;     // no se usa
    P.target_count[PLATFORM] = 75;
    P.target_count[OBSTACLE] = 15;
    P.target_count[ENEMY]    = 20;
 
    // Funcion objetivo: Z = alpha * D + beta * T   (alpha + beta = 1)
    P.alpha = 0.9;                // 70% importan las cantidades
    P.beta  = 0.1;                // 30% importa la forma del nivel
    P.want_variation = true;      // true = nivel con desniveles (no lineal)
 
    // Probar el greedy con un punto inicial
    int start = 5;
    Level level = greedy(P, start);
 
    string path = "../out/greedy_no_lineal.txt";
    saveLevel(level, P, path);
 
    cout << "Z           = " << evaluate(level, P) << "\n";
    cout << "Linealidad  = " << linearity(level) << "\n";
    cout << "Plataformas = " << countObjects(level, PLATFORM) << " / " << P.target_count[PLATFORM] << "\n";
    cout << "Obstaculos  = " << countObjects(level, OBSTACLE) << " / " << P.target_count[OBSTACLE] << "\n";
    cout << "Enemigos    = " << countObjects(level, ENEMY)    << " / " << P.target_count[ENEMY]    << "\n";
    cout << "Guardado en " << path << "\n";
 
    return 0;
}