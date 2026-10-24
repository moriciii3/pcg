#include "level.h"
#include <cmath>
#include <fstream>
using namespace std;

// Cuenta cuantos objetos del tipo k hay instanciados (por ejemplo cuantas plataformas hay de momento en el sistema)
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

// Calcula la diferencia entre la linealidad real vs la que se solicito
double heightTerm(const Level& level, const Params& P) {
    return abs(linearity(level) - P.L_target);
}

// Suma todos los objetos pedidos por el diseñador, todas las plataformas, enemigos y obstaculos
int totalTarget(const Params& P){
    int t = 0;
    for(int k = 1; k < NTYPES; k++){
        t += P.target_count[k];
    }
    return t;
}

// Evalua la funcion objetivo y retorna su valor
double evaluate(const Level& level, const Params& P) {
    double D = 0.0;
    for (int k = 1; k < NTYPES; k++)
        D += abs(countObjects(level, k) - P.target_count[k]);
    D /= totalTarget(P);
    return P.alpha * D + P.beta * heightTerm(level, P);
}

// ===================================================================
// PATRONES DE COLUMNA
// ===================================================================

// Pone una plataforma en (i, j) y, si se pide, un objeto encima (fila j+1).
// on_top = EMPTY si no va nada encima
void place(Level& level, int i, int j, int on_top) {
    level.cells[i][j] = PLATFORM;
    if (on_top != EMPTY && j + 1 < level.H) level.cells[i][j + 1] = on_top;
}

// Deja la columna i vacia
void clearColumn(Level& level, int i) {
    for (int j = 0; j < level.H; j++) level.cells[i][j] = EMPTY;
}

// ===================================================================
// RESTRICCIONES
// ===================================================================

// Retorna la máxima altura a la que se puede llegar con un determinado desplazamiento de salto en x llamado dx
// Se utiliza la fórmula de la parábola para poder calcular con los vertices
double jumpHeight(const Params& P, int dx) {
    double D = P.max_x_jump + 1;
    if (dx <= D / 2.0) return P.max_y_jump;
    return 4.0 * P.max_y_jump * dx * (D - dx) / (D * D);
}

// Retorna True si el salto se puede realizar, el salto es un desplazamiento dx en X y dy en Y.
bool canJump(const Params& P, int dx, int dy) {
    if (dx < 1) return false;

    // Calculamos la altura maxima basandonos en el salto en X, ya que es una parábola
    // Utilizamos floor para redondear hacia abajo, un salto de 4,90 no es realizable, uno de 4.0 si.
    return dy <= floor(jumpHeight(P, dx) + 1e-9);
}

// Retorna la fila en donde se encuentra la plataforma de la columna i, si no hay devuelve -1
static int platformRow(const Level& level, int i) {
    for (int j = 0; j < level.H; j++)
        if (level.cells[i][j] == PLATFORM) return j;
    return -1;
}

// Retorna lo que hay encima de una casilla (Si se excede del mapa devuelve EMPTY)
static int onTopOf(const Level& level, int i, int row) {
    if (row + 1 >= level.H) return EMPTY;
    return level.cells[i][row + 1];
}

// Revisa las restricciones en en la ventana de columnas entre lastCol y firstCol
bool isFeasible(const Level& level, const Params& P, int lastCol, int firstCol) {
    if (lastCol < 0) lastCol = level.W - 1;
    bool complete = (lastCol == level.W - 1);

    int prevCol = -1;   // columna de la ultima plataforma pisable
    int prevRow = -1;   // fila de esa plataforma (desde donde se salta)

    for (int i = firstCol; i <= lastCol; i++) {
        // R1: a lo mas una plataforma por columna
        int platforms = 0;
        for (int j = 0; j < level.H; j++)
            if (level.cells[i][j] == PLATFORM) platforms++;
        if (platforms > 1) return false;

        // R2: todo enemigo u obstaculo esta justo encima de una plataforma
        // (R3, un objeto por casilla, se cumple solo: cada casilla guarda un valor)
        for (int j = 0; j < level.H; j++) {
            int k = level.cells[i][j];
            if (k == ENEMY || k == OBSTACLE) {
                if (j == 0 || level.cells[i][j - 1] != PLATFORM) return false;
            }
        }

        int row = platformRow(level, i);
        int top = (row == -1) ? EMPTY : onTopOf(level, i, row);

        // R5 (inicio): la columna 0 tiene una plataforma libre
        if (i == 0 && (row == -1 || top != EMPTY)) return false;

        if (row == -1) continue;

        // Obstaculo: no se pisa, pero su plataforma debe quedar al alcance del salto
        if (top == OBSTACLE) {
            if (prevCol != -1 && !canJump(P, i - prevCol, row - prevRow)) return false;
            continue;
        }

        // R4: el salto desde la ultima plataforma pisable debe estar en R.
        // Si hay un enemigo, se aterriza sobre el (fila + 1) para derrotarlo.
        if (prevCol != -1) {
            int arrival = row + (top == ENEMY ? 1 : 0);
            if (!canJump(P, i - prevCol, arrival - prevRow)) return false;
        }
        prevCol = i;
        prevRow = row;
    }

    if (complete) {
        // R5 (fin): la ultima columna tiene una plataforma libre
        int row = platformRow(level, level.W - 1);
        if (row == -1 || onTopOf(level, level.W - 1, row) != EMPTY) return false;
    } else {
        // Nivel parcial: el hueco abierto debe poder cerrarse todavia.
        // Lo mas facil de alcanzar es una plataforma libre en la fila 0 de la columna siguiente.
        if (!canJump(P, lastCol + 1 - prevCol, 0 - prevRow)) return false;
    }
    return true;
}

// Guarda el nivel en un txt, en el header van los parametros, abajo los resultados,
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