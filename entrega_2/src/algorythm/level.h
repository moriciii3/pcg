#pragma once
#include <vector>
#include <string>
using namespace std;

enum Type { EMPTY = 0, PLATFORM = 1, OBSTACLE = 2, ENEMY = 3 };
const int NTYPES = 4;

struct Params {
    // Parametros del salto maximo
    int max_x_jump, max_y_jump;

    // Parametros del mapa: W := Ancho del mapa | H := Alto del mapa
    int W, H;

    // Lo que pide el diseñador
    int target_count[NTYPES];   // cantidad pedida de cada tipo (EMPTY no se usa)
    double L_target;            // linealidad buscada: 1 = sigue una recta, 0.5 = sube y baja mucho

    // Parametros funcion objetivo: Z = alpha * D + beta * |L - L_target|   (alpha + beta = 1)
    double alpha;               // importancia de las cantidades
    double beta;                // importancia de la linealidad
};

struct Level {
    int W, H;
    vector<vector<int>> cells;  // cells[i][j]: columna i, fila j (fila 0 = abajo)
    Level(int W, int H) : W(W), H(H), cells(W, vector<int>(H, EMPTY)) {}
};

// ---- level.cpp
// Cuenta cuantos objetos hay del tipo k
int countObjects(const Level& n, int k);

// Linealidad: ajusta una recta a las plataformas y mide
// la distancia promedio a ella. Entre 0 y 1, 1 = las plataformas siguen una recta
double linearity(const Level& n);

// Termino de linealidad de la funcion objetivo: |L - L_target|
double heightTerm(const Level& n, const Params& P);

// Suma de lo pedido, para normalizar las cantidades
int totalTarget(const Params& P);

// Funcion objetivo Z
double evaluate(const Level& n, const Params& P);

// Altura maxima de llegada al saltar dx columnas (parabola del salto)
double jumpHeight(const Params& P, int dx);

// true si el salto (dx, dy) es alcanzable, es decir, (dx, dy) pertenece a R
bool canJump(const Params& P, int dx, int dy);

// Revisa las restricciones en una ventana de columnas
// Sin argumentos revisa el nivel completo.
// Greedy: isFeasible(level, P, i)            -> columnas 0..i
// Hill Climbing: isFeasible(level, P, n, p)  -> ventana p..n
bool isFeasible(const Level& n, const Params& P, int lastCol = -1, int firstCol = 0);

// Pone una plataforma en (i, j) y, si se pide, un objeto encima (fila j+1)
void place(Level& level, int i, int j, int on_top);

// Deja la columna i vacia
void clearColumn(Level& level, int i);

// Guarda el nivel en un txt (parametros arriba, casillas abajo)
void saveLevel(const Level& n, const Params& P, const string& path);

// ---- greedy.cpp
// Genera una solucion greedy a partir de un punto inicial (fila de la primera plataforma)
Level greedy(const Params& P, int startingPoint);

// ---- hill_climbing.cpp
// Recibe una solucion inicial y la mejora
Level hillClimbing(Level initial, const Params& P);