#include "level.h"
#include <cmath>
#include <fstream>
#include <algorithm>
#include <climits>
using namespace std;

// Cuenta cuantos objetos del tipo k hay instanciados (por ejemplo cuantas plataformas hay de momento en el sistema)
int countObjects(const Level& level, int k) {
    int total = 0;
    for (int i = 0; i < level.W; i++)
        for (int j = 0; j < level.H; j++)
            if (level.cells[i][j] == k) total++;
    return total;
}

// Evalua el polinomio interpolado (de forma baricentrica) en el valor X
// Berrut, J.-P., & Trefethen, L. N. (2004). Barycentric Lagrange interpolation. SIAM Review, 46(3), 501–517. https://doi.org/10.1137/S0036144502417715
static double barycentric(const vector<pair<double, double>>& g, double x) {
    double num = 0.0, den = 0.0;
    for (size_t k = 0; k < g.size(); k++) {
        double diff = x - g[k].first;
        if (fabs(diff) < 1e-12) return g[k].second;
        double w = 1.0;
        for (size_t m = 0; m < g.size(); m++)
            if (m != k) w /= (g[k].first - g[m].first);
        num += w / diff * g[k].second;
        den += w / diff;
    }
    return num / den;
}

// Crea una curva guiada una sola vez, evalua el polinomio interpolador en todo el mapa y guarda en P.shape
// OBS: P.guide son los puntos de interpolacion
void prepareShape(Params& P) {
    P.shape.assign(P.W, 0.0);
    if (P.guide.size() < 2) {
        for (int i = 0; i < P.W; i++) P.shape[i] = i;
        return;
    }
    double xMin = P.guide.front().first, xMax = P.guide.back().first;   // puntos ordenados por x
    for (int i = 0; i < P.W; i++)
        P.shape[i] = barycentric(P.guide, max(xMin, min(xMax, (double)i)));
}

// Linealidad (como en Launchpad, pero contra la curva guia en vez de una recta):
// que tan cerca siguen las plataformas la curva guia g.
// 1) Se toma un punto (i, h_i) por cada columna con plataforma.
// 2) Se ajusta la curva por minimos cuadrados:
//    - Con curva guia (2 o mas puntos): h = a + g(i). Solo se sube o baja la curva (a);
//      su forma y el tamano de sus subidas y bajadas los definen los puntos del diseñador.
//      (Si se permitiera achatarla, una curva achatada es una linea plana y cualquier
//      nivel plano "se pareceria" a cualquier curva.)
//    - Sin puntos: h = a + b*i, una recta con cualquier pendiente (la linealidad de siempre).
// 3) Se calcula la distancia promedio de los puntos a la curva ajustada y se normaliza.
// Resultado entre 0 y 1: 1 = las plataformas siguen la curva.
double linearity(const Level& level, const Params& P) {
    // Guardamos en vectores las alturas reales v/s las alturas de referencia P.shape
    //  gs = altura que marca la curva guia
    //  hs = altura real de la plataforma
    vector<double> gs, hs;
    for (int i = 0; i < level.W; i++) {
        for (int j = 0; j < level.H; j++) {
            if (level.cells[i][j] == PLATFORM) {
                gs.push_back(P.shape.empty() ? (double)i : P.shape[i]);
                hs.push_back(j);
                break;
            }
        }
    }


    int n = gs.size(); // Cantidad de puntos de guia
    if (n < 2) return 1.0;

    // Calculamos alturas promedio
    //  mg = altura promedio curva guia
    //  mh = altura promedio real plataformas
    double mg = 0, mh = 0;
    for (int t = 0; t < n; t++) {
        mg += gs[t];
        mh += hs[t];
    }

    mg = mg/n;
    mh = mh/n;

    // Calculamos las diferencias de cada punto v/s el promedio
    //  sgg = diferencia acumulada puntos guias
    //  sgh = diferencia acumulada plataformas colocadas
    double sgh = 0, sgg = 0;
    for (int t = 0; t < n; t++) {
        sgh += (gs[t] - mg) * (hs[t] - mh);
        sgg += (gs[t] - mg) * (gs[t] - mg);
    }

    // Calculamos coeficientes a y b de la recta
    // Si no hay puntos guia, va sin pendiente
    double b;
    if (sgg > 0) {
        b = sgh/sgg;
    } else {
        b = 0.0;
    }
    
    if (P.guide.size() >= 2) b = 1.0; // con curva guia: solo se sube o baja
    double a = mh - b * mg;

    // Distancia promedio a la curva ajustada
    double dist = 0;
    for (int t = 0; t < n; t++) dist += abs(hs[t] - (a + b * gs[t]));
    dist /= n;

    // La mayor distancia promedio posible es cerca de la mitad del alto del mapa
    double maxDist = (level.H - 1) / 2.0;
    return max(0.0, 1.0 - dist / maxDist);
}

// Calcula la diferencia entre la linealidad real vs la que se solicito
double heightTerm(const Level& level, const Params& P) {
    return abs(linearity(level, P) - P.L_target);
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

// TRAYECTORIA DEL SALTO
// Definimos un struct que nos ayude a armar las curvas de los saltos y revisar si hay colisiones que intercepten la factibilidad
struct Trajectory {
    double a; // altura maxima del salto en y
    double s; // desplazamiento en x del salto
    double k; // velocidad de caida
    bool fall; // true si se deja caer, false si es un salto
};


// ¿A que altura va un personaje si lleva t columnas de vuelo en un salto?
static double trajY(const Trajectory& tr, double t) {
    if (tr.fall) { // CAIDA: no salta, se deja caer
        if (t <= 0.5) return 0.0;               // todavia camina sobre la plataforma
        return -tr.k * (t - 0.5) * (t - 0.5);   // ya salio del borde: va cayendo
    }
    // SALTO: sube y despues baja
    return tr.a * t * (2.0 * tr.s - t) / (tr.s * tr.s);
}

// Arma el conjunto de saltos que sabe hacer el jugador: saltos de distintas alturas y largos,
// y caidas desde el borde de la plataforma. No depende del nivel, solo de max_x_jump y max_y_jump.
// Despues, jumpPath prueba estos saltos uno por uno hasta encontrar uno que llegue sin chocar.
static const vector<Trajectory>& trajectories(const Params& P) {
    static vector<Trajectory> family;

    // Verificamos si ya se realizo el calculo antes (tiene complejidad cuadratica, ahorramos el recalculo asi)
    static int cachedX = -1, cachedY = -1;
    if (cachedX == P.max_x_jump && cachedY == P.max_y_jump) return family;
    family.clear();

    // Calculamos los valores maximos de salto (vertice y desplazamiento en x)
    double D = P.max_x_jump + 1;
    double kMin = 4.0 * P.max_y_jump / (D * D);

    // Saltos: el salto maximo primero, porque es el que mas lejos llega
    for (int a = P.max_y_jump; a >= 1; a--) {
        double sMax = sqrt(a / kMin);
        for (double s = sMax; s >= 0.5 - 1e-9; s -= 0.5)
            family.push_back({(double)a, s, 0.0, false});
    }
    // Caidas: de mas larga a mas vertical
    for (double m : {1.0, 2.0, 4.0, 16.0})
        family.push_back({0.0, 0.0, kMin * m, true});

    // Cacheamos los datos para no recalcular
    cachedX = P.max_x_jump;
    cachedY = P.max_y_jump;
    return family;
}

// Determina si la casilla (i,j) esta vacia
static bool freeCell(const Level& level, int i, int j, int ignoreI, int ignoreJ) {
    if (i < 0 || i >= level.W || j < 0) return false; // Fuera del mapa lateralmente (no permitido)
    if (j >= level.H) return true; // Fuera del mapa por encima (permitido)
    if (i == ignoreI && j == ignoreJ) return true; // Si es la casilla del jugador, va libre
    return level.cells[i][j] == EMPTY;
}

// Detecta si el avatar esta libre
static bool avatarFree(const Level& level, double x, double y, int ignoreI, int ignoreJ) {
    int i  = (int)floor(x);
    int j0 = (int)floor(y + 1e-9), j1 = (int)floor(y + 1.0 - 1e-6);
    for (int j = j0; j <= j1; j++)
        if (!freeCell(level, i, j, ignoreI, ignoreJ)) return false;
    return true;
}

// Revisa si existe un salto de la familia que va desde la plataforma de
// salida (columna fromCol, fila fromRow) hasta aterrizar en la columna toCol
// a la fila landRow (fila donde quedan los pies del avatar), sin chocar.
// Aterrizaje: apenas el centro del avatar entra en la columna de destino con
// los pies a la altura de llegada o mas arriba, puede bajar en vertical hasta
// aterrizar (control en el aire).
bool jumpPath(const Level& level, const Params& P, int fromCol, int fromRow, int toCol, int landRow) {
    int dx = toCol - fromCol;
    int startY = fromRow + 1; // fila de los pies parado sobre la plataforma
    int dy = landRow - startY;
    if (!canJump(P, dx, dy)) return false; // filtramos rapido cuando ni el mejor salto llega

    int ignoreI = fromCol, ignoreJ = startY; // enemigo derrotado en la salida, si lo hay
    const double step = 0.1;
    const int steps = (int)llround(dx / step);

    for (const Trajectory& tr : trajectories(P)) {
        for (int n = 1; n <= steps; n++) {
            double t = n * step;
            double x = fromCol + 0.5 + t; // centro del avatar
            double y = startY + trajY(tr, t); // pies del avatar

            // Ya sobre la columna de destino y a la altura de llegada: bajar en vertical
            if ((int)floor(x + 1e-9) == toCol && y >= landRow - 1e-6) {
                bool ok = true;
                for (int j = (int)floor(y + 1e-6); j >= landRow && ok; j--)
                    ok = freeCell(level, toCol, j, ignoreI, ignoreJ);
                if (ok) return true;
                break;
            }
            if (!avatarFree(level, x, y, ignoreI, ignoreJ)) break; // Colision, provamos otra curva
        }
    }
    return false;
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
//   lastCol = W-1 (o -1): hasta el final del nivel, se revisa tambien el fin (R5).
//   firstCol > 0        : ventana. firstCol debe ser una plataforma pisable ya revisada.
//   building = true     : nivel en construccion (greedy). Las columnas despues de lastCol
//                         estan vacias, y se revisa que el hueco abierto se pueda cerrar.
bool isFeasible(const Level& level, const Params& P, int lastCol, int firstCol, bool building) {
    if (lastCol < 0) lastCol = level.W - 1;
    bool complete = (lastCol == level.W - 1);

    int prevCol = -1;   // columna de la ultima plataforma pisable
    int prevRow = -1;   // fila de esa plataforma (desde donde se salta)
    int obsMin = INT_MAX, obsMax = INT_MIN;   // filas de los obstaculos desde la ultima pisable

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

        // Obstaculo: no se pisa. Para que aporte al recorrido debe quedar en el camino:
        //   - su plataforma no puede estar mas abajo que la ultima plataforma pisable
        //     (si no, queda hundido y el jugador pasa por encima caminando, sin notarlo);
        //   - ni mas arriba que la mas alta de las dos pisables vecinas
        //     (si no, queda flotando y el jugador pasa por debajo). Esto se revisa al
        //     llegar a la siguiente pisable.
        // Ademas debe quedar al alcance del salto.
        if (top == OBSTACLE) {
            if (prevCol != -1) {
                if (row < prevRow) return false;
                if (!canJump(P, i - prevCol, row - prevRow)) return false;
                obsMin = min(obsMin, row);
                obsMax = max(obsMax, row);
            }
            continue;
        }

        // R4: debe existir un salto sin choques desde la ultima plataforma pisable.
        // Si hay un enemigo, se aterriza sobre el (los pies quedan en fila + 2) para derrotarlo.
        if (prevCol != -1) {
            // Obstaculos entre la pisable anterior y esta: deben quedar entre ambas alturas
            if (obsMin != INT_MAX && obsMax > max(prevRow, row)) return false;

            int landRow = row + 1 + (top == ENEMY ? 1 : 0);
            if (!jumpPath(level, P, prevCol, prevRow, i, landRow)) return false;
        }
        prevCol = i;
        prevRow = row;
        obsMin = INT_MAX;
        obsMax = INT_MIN;
    }

    if (complete) {
        // R5 (fin): la ultima columna tiene una plataforma libre
        int row = platformRow(level, level.W - 1);
        if (row == -1 || onTopOf(level, level.W - 1, row) != EMPTY) return false;
    } else if (building) {
        // Nivel en construccion: el hueco abierto debe poder cerrarse todavia, es decir,
        // debe existir alguna fila de la columna siguiente donde poner una plataforma
        // libre y llegar a ella sin chocar. Se prueba primero cerca de la altura actual.
        int next = lastCol + 1;
        bool closable = false;
        for (int d = 0; d < level.H && !closable; d++) {
            int candidates[2] = {prevRow - d, prevRow + d};
            for (int c = 0; c < (d == 0 ? 1 : 2) && !closable; c++) {
                int r = candidates[c];
                if (r < 0 || r >= level.H) continue;
                // la siguiente pisable debe dejar a los obstaculos pendientes entre ambas alturas
                if (obsMin != INT_MAX && obsMax > max(prevRow, r)) continue;
                if (jumpPath(level, P, prevCol, prevRow, next, r + 1))
                    closable = true;
            }
        }
        if (!closable) return false;
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
    f << "seed " << P.seed << "\n";
    if (P.guide.size() >= 2) {
        f << "guide";
        for (auto& g : P.guide) f << " " << g.first << " " << g.second;
        f << "\n";
    }
    f << "target_platform " << P.target_count[PLATFORM] << "\n";
    f << "target_obstacle " << P.target_count[OBSTACLE] << "\n";
    f << "target_enemy " << P.target_count[ENEMY] << "\n";

    // Encabezado: resultados
    f << "Z " << evaluate(level, P) << "\n";
    f << "linearity " << linearity(level, P) << "\n";
    f << "platform " << countObjects(level, PLATFORM) << "\n";
    f << "obstacle " << countObjects(level, OBSTACLE) << "\n";
    f << "enemy " << countObjects(level, ENEMY) << "\n";
    f << "---\n";

    // Casillas
    for (int i = 0; i < level.W; i++)
        for (int j = 0; j < level.H; j++)
            f << "(" << i << "," << j << ") -> " << typeName[level.cells[i][j]] << "\n";
}