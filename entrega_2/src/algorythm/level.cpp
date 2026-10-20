#include "level.h"
#include <bits/stdc++.h>
using namespace std;

// o_k = suma de x_ijk sobre todas las casillas
// TODO: Hacer que esto se evite, la idea es hacer un diccionario que vaya aumentando cada vez que asigne, asi evitamos O(n2)
int countObjects(const Level& level, int k) {
    int total = 0;
    for (int i = 0; i < level.W; i++)
        for (int j = 0; j < level.H; j++)
            if (level.cells[i][j] == k) total++;
    return total;
}

// L = Entropia normalizada de las alturas de las plataformas.
// Se cuenta solo la plataforma más alta de cada columna.
// TODO: Ver si podemos evitar el O(n2) calculandolo en tiempo de construccion
double linearity(const Level& level) {
    vector<int> a(level.H, 0);
    int total = 0;
    for (int i = 0; i < level.W; i++) {
        for (int j = 0; j < level.H; j++) {
            if (level.cells[i][j] == PLATFORM) { a[j]++; total++; break; }
        }
    }
    if (total == 0) return 0.0;

    double entropy = 0.0;
    for (int j = 0; j < level.H; j++) {
        if (a[j] == 0) continue; // p = 0 no aporta
        double q = (double)a[j] / total;
        entropy -= q * log2(q);
    }
    return entropy / log2(level.H);
}

// Funcion de evaluacion
double evaluate(const Level& level, const Params& P) {
    double Z = 0.0;
    for (int k = 1; k < NTYPES; k++)
        Z += abs(countObjects(level, k) - P.target_count[k]);
    Z = P.alpha * Z + P.beta * linearity(level);
    return Z;
}

// Evalua la factibilidad de una solucion
bool isFeasible(const Level& level, const Params& P){
    // TODO: Aplicar restricciones
    return true;
}