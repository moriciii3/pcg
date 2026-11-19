#pragma once
#include <vector>
#include <string>
#include <random>
#include <utility>
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
    vector<pair<double, double>> guide;  // puntos de la curva guia (columna x, fila y). Menos de 2 = recta
    vector<double> shape;       // curva guia evaluada en cada columna (se calcula con prepareShape)

    // Parametros funcion objetivo: Z = alpha * D + beta * |L - L_target|   (alpha + beta = 1)
    double alpha;               // importancia de las cantidades
    double beta;                // importancia de la linealidad

    // Parametros de la busqueda
    int seed;                   // semilla: fila inicial de cada reinicio y desempates (se lee de la instancia)
    int restarts;               // cantidad de reinicios (greedy + Hill Climbing)
};

struct Level {
    int W, H;
    vector<vector<int>> cells;  // cells[i][j]: columna i, fila j (fila 0 = abajo)
    Level(int W, int H) : W(W), H(H), cells(W, vector<int>(H, EMPTY)) {}
};

// ---- level.cpp
// Cuenta cuantos objetos hay del tipo k
int countObjects(const Level& n, int k);

// Curva guia: polinomio que pasa por los puntos de la instancia (forma baricentrica).
// Se evalua una vez en cada columna y se guarda en P.shape.
void prepareShape(Params& P);

// Linealidad: ajusta la curva guia a las plataformas por minimos cuadrados (subiendola o
// bajandola) y mide la distancia promedio a ella. Entre 0 y 1, 1 = las plataformas siguen
// la curva. Con 2 puntos la curva es una recta; sin puntos, una recta de cualquier pendiente.
double linearity(const Level& n, const Params& P);

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

// true si existe un salto (de la familia de trayectorias) que va desde la plataforma
// (fromCol, fromRow) hasta dejar al avatar en la fila landRow de la columna toCol sin chocar
bool jumpPath(const Level& n, const Params& P, int fromCol, int fromRow, int toCol, int landRow);

// Revisa las restricciones en una ventana de columnas.
// Sin argumentos revisa el nivel completo.
// Greedy: isFeasible(level, P, i, p, true)   -> desde la ultima pisable p hasta i, en construccion
// Hill Climbing: isFeasible(level, P, n, p)  -> ventana p..n
bool isFeasible(const Level& n, const Params& P, int lastCol = -1, int firstCol = 0, bool building = false);

// Pone una plataforma en (i, j) y, si se pide, un objeto encima (fila j+1)
void place(Level& level, int i, int j, int on_top);

// Deja la columna i vacia
void clearColumn(Level& level, int i);

// Guarda el nivel en un txt (parametros arriba, casillas abajo)
void saveLevel(const Level& n, const Params& P, const string& path);

// ---- greedy.cpp
// Genera una solucion greedy a partir de un punto inicial (fila de la primera plataforma)
Level greedy(const Params& P, int startingPoint, mt19937& rng);

// ---- hill_climbing.cpp
// Recibe una solucion inicial y la mejora
Level hillClimbing(Level initial, const Params& P);