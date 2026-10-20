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

// Termino de forma: entre 0 y 1, siempre se minimiza
double heightTerm(const Level& level, const Params& P) {
    double L = linearity(level);
    return P.want_variation ? 1.0 - L : L;
}

// Suma de lo pedido, para normalizar
int totalTarget(const Params& P) {
    int t = 0;
    for (int k = 1; k < NTYPES; k++) t += P.target_count[k];
    return t;
}

// Funcion de evaluacion
double evaluate(const Level& level, const Params& P) {
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k]);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
}

// Evalua la factibilidad de una solucion
bool isFeasible(const Level& level, const Params& P){
    // TODO: Aplicar restricciones
    return true;
}

// Guarda el nivel en un txt: arriba los parametros y resultados,
// abajo una linea por casilla con el formato  (i,j) -> tipo
void saveLevel(const Level& level, const Params& P, const string& path) {
    const string typeName[NTYPES] = {"empty", "platform", "obstacle", "enemy"};
    ofstream f(path);
 
    // Encabezado: parametros
    f << "W " << P.W << "\n";
    f << "H " << P.H << "\n";
    f << "max_x_jump " << P.max_x_jump << "\n";
    f << "max_y_jump " << P.max_y_jump << "\n";
    f << "alpha " << P.alpha << "\n";
    f << "beta " << P.beta << "\n";
    f << "target_platform " << P.target_count[PLATFORM] << "\n";
    f << "target_obstacle " << P.target_count[OBSTACLE] << "\n";
    f << "target_enemy " << P.target_count[ENEMY] << "\n";
 
    // Encabezado: resultados
    f << "Z " << evaluate(level, P) << "\n";
    f << "linearity " << linearity(level) << "\n";
    f << "platform " << countObjects(level, PLATFORM) << "\n";
    f << "obstacle " << countObjects(level, OBSTACLE) << "\n";
    f << "enemy " << countObjects(level, ENEMY) << "\n";
    f << "---\n";
 
    // Casillas
    for (int i = 0; i < level.W; i++)
        for (int j = 0; j < level.H; j++)
            f << "(" << i << "," << j << ") -> " << typeName[level.cells[i][j]] << "\n";
}