# Entrega final - Generación Procedural de Plataformas
Bruno Morici
ROL 202373555-8

## Correcciones sobre entrega anterior
### 1. Intersección en saltos
En la entrega anterior, se generaban *falsos positivos* en los saltos. Si un salto era posible, pero había un obstáculo en la trayectoria de ese salto, el algoritmo lo consideraba como un salto válido.

Esto se solucionó revisando la trayectoria completa del salto, y no solo el punto de llegada. El salto se recorre en pasos pequeños, columna por columna, y cada casilla por la que pasa el avatar debe estar vacía hasta tocar la plataforma de llegada. Si la trayectoria choca con una plataforma, un obstáculo o un enemigo antes de aterrizar, el salto se descarta.

Además, en vez de usar una sola parábola, se considera una familia de saltos con distintas alturas y largos (desde dejarse caer hasta el salto máximo). Un salto se considera válido si al menos una de estas trayectorias llega sin chocar, lo que también evita falsos negativos: saltos que se rechazaban con la parábola máxima, pero que un salto más corto o más bajo sí logra.

### 2. Saltos fuera del mapa
En la entrega anterior, las plataformas no podían ubicarse en la fila más alta del mapa, y la trayectoria de los saltos no consideraba el borde superior, por lo que el jugador podía saltar fuera del mapa.

Esto se solucionó permitiendo plataformas hasta la fila 16 y considerando filas vacías por encima del mapa al revisar los saltos. Estas filas no se agregan al nivel generado, que sigue teniendo 16 filas: solo se usan para que el avatar tenga espacio al saltar desde las plataformas más altas.

Como el avatar sube como máximo `salto_maximo_y` casillas sobre su plataforma, basta con considerar:

altura_considerada = altura_mapa + salto_maximo_y

Por ejemplo, si el mapa tiene 16 de altura y el salto máximo es de 4, los saltos se revisan con 20 filas, pero el nivel se genera y se guarda con 16.

### 3. Curva guía
(Estoy orgulloso de esto)

Se implementó una curva de guía que permite al diseñador darle una forma establecida al nivel.

El diseñador define una *serie de puntos*, estos puntos deben ir separados en el eje X, es decir, no pueden haber puntos encima de otros. Luego a los puntos se les realiza una *interpolación baricéntrica* y se obtiene un polinomio que define la curva. (Función liviana, continua y derivable).

Finalmente, hay un parámetro llamado `L_target` que define qué tan cerca el nivel debe estar de la curva guía. Se calcula la diferencia entre un punto a colocar con la curva mediante *mínimos cuadrados*, y se penaliza la diferencia. Esto permite que el nivel generado tenga una forma establecida por el diseñador, pero sin ser demasiado rígido.
## Cómo ejecutarlo

### 1. Definir instancia
En la carpeta `src/instances/` se crean archivos `.txt` que definen los parámetros de la instancia a generar. Por ejemplo, el archivo `inst1.txt` contiene:

```
# Mapa
W 150
H 16

# Salto
max_x_jump 6
max_y_jump 4

# Cantidades pedidas
platforms 75
enemies 20
obstacles 15

# Funcion objetivo
alpha 0.7
beta 0.3
L_target 0.8

# Puntos interpolacion (x = columna, y = fila)
x 0 y 3
x 52 y 12
x 104 y 4
x 149 y 9

# Busqueda
seed 1
restarts 16
```

- **Puntos de interpolación (opcionales):** definen la curva guía que siguen las plataformas. Con 2 puntos la curva es una recta, y sin puntos se usa una recta con cualquier pendiente.
- **Búsqueda (opcional):** `seed` es la semilla (por defecto 1) y `restarts` es la cantidad de reinicios (por defecto 16). Con la misma semilla se obtiene siempre el mismo nivel.

### 2. Compilar y ejecutar
Entrar a la carpeta `src/algorythm/` y ejecutar:

```bash
make run INST=../instances/inst1.txt
```

Esto compila el programa y lo ejecuta con la instancia indicada. Para borrar el ejecutable se usa `make clean`.

Esto guarda la mejor solución en la carpeta `src/out/`, en dos archivos:
- `inst1_seed1_hc.txt`: el resultado final, después del Hill Climbing.
- `inst1_seed1_greedy.txt`: la solución del Greedy de la que partió.

### 3. Generar imagen
Entrar a la carpeta `src/format/` y ejecutar:

```bash
python3 main.py                              # grafica todos los .txt de src/out/
python3 main.py ../out/inst1_seed1_hc.txt    # grafica solo ese archivo
```

Esto transforma cada mapa en una imagen `.png`, que se guarda en `src/out/` al lado de su `.txt`. Si la instancia tiene puntos de interpolación, la imagen también muestra la curva guía.