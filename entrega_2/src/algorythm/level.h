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
    int W,H;

    // Cantidad pedida por el diseñador de cada objeto (O[EMPTY] no se usa)
    int target_count[NTYPES];

    // Parametros funcion objetivo
    bool want_variation;
    double alpha;
    double beta;
};

struct Level {
    int W, H;
    vector<vector<int>> cells;
    Level(int W, int H) : W(W), H(H), cells(W, vector<int>(H, EMPTY)) {}
};

// ---- level.cpp ----
// Cuenta cuantos objetos hay del tipo k
int countObjects(const Level& level, int k);

// Calcula la linealidad del nivel con la entropia
double linearity(const Level& level);

double heightTerm(const Level& level, const Params& P);

int totalTarget(const Params& P);

// Evalua la funcion objetivo y entrega un valor
double evaluate(const Level& level, const Params& P);

// Evalua restricciones, sirve para HC
bool isFeasible(const Level& level, const Params& P);

// Guarda en nivel en un txt que luego es interpretado en Python
void saveLevel(const Level& level, const Params& P, const string& path);

// ---- greedy.cpp ----
// Genera una solucion greedy mediante un punto inicial
Level greedy(const Params& P, int startingPoint);

// ---- hill_climbing.cpp ----
// Recibe una solucion inicial y la explota
Level hillClimbing(Level initial_solution, const Params& P);