# Compila y ejecuta el generador de escenas.
#   make                                   -> compila (crea el ejecutable pcg)
#   make run                               -> compila y ejecuta con inst1
#   make run INST=../instances/inst2.txt   -> compila y ejecuta con otra instancia
#   make clean                             -> borra el ejecutable

CXX      = g++
CXXFLAGS = -O2 -std=c++17
FUENTES  = level.cpp greedy.cpp hill_climbing.cpp main.cpp
INST    ?= ../instances/inst1.txt

pcg: $(FUENTES) level.h
	$(CXX) $(CXXFLAGS) $(FUENTES) -o pcg

run: pcg
	./pcg $(INST)

clean:
	rm -f pcg

.PHONY: run clean