#include "level.h"
#include <cmath>
#include <fstream>
using namespace std;

int countObjects(const Level& level, int k) {
    int total = 0;
    for (int i = 0; i < level.W; i++)
        for (int j = 0; j < level.H; j++)
            if (level.cells[i][j] == k) total++;
    return total;
}

// Linealidad (como en Launchpad): que tan cerca siguen las plataformas una linea recta.
// 1) Se toma un punto (i, j) por cada columna con plataforma.
// 2) Se ajusta la recta j = a + b*i por minimos cuadrados.
// 3) Se calcula la distancia promedio de los puntos a la recta y se normaliza.
// Resultado entre 0 y 1: 1 = las plataformas siguen una recta, 0 = se alejan mucho.
double linearity(const Level& level) {
    vector<double> xs, ys;
    for (int i = 0; i < level.W; i++) {
        for (int j = 0; j < level.H; j++) {
            if (level.cells[i][j] == PLATFORM) { xs.push_back(i); ys.push_back(j); break; }
        }
    }
    int n = xs.size();
    if (n < 2) return 1.0;

    // Recta de minimos cuadrados
    double mx = 0, my = 0;
    for (int t = 0; t < n; t++) { mx += xs[t]; my += ys[t]; }
    mx /= n; my /= n;
    double sxy = 0, sxx = 0;
    for (int t = 0; t < n; t++) { sxy += (xs[t] - mx) * (ys[t] - my); sxx += (xs[t] - mx) * (xs[t] - mx); }
    double b = (sxx > 0) ? sxy / sxx : 0.0;
    double a = my - b * mx;

    // Distancia promedio a la recta
    double dist = 0;
    for (int t = 0; t < n; t++) dist += abs(ys[t] - (a + b * xs[t]));
    dist /= n;

    // La mayor distancia promedio posible es cerca de la mitad del alto del mapa
    double maxDist = (level.H - 1) / 2.0;
    return max(0.0, 1.0 - dist / maxDist);
}


// Retorna el valor absoluto de la diferencia entre la linealidad obtenida y la solicitada por el usuario
double heightTerm(const Level& level, const Params& P) {
    return abs(linearity(level) - P.L_target);
}

// Retorna el total de objetos solicitados por el diseñador (obstaculos, plataformas y enemigos)
int totalTarget(const Params& P){
    int t = 0;
    for(int k = 1; k < NTYPES; k++) t += P.target_count[k];
    return t;
}

double evaluate(const Level& level, const Params& P) {
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k]);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
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
    f << "L_target " << P.L_target << "\n";
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