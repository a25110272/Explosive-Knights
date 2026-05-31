CXX = g++
CXXFLAGS = -std=c++17 -Wall
SRC = src/main.cpp
OUT = bin/JuegoProyecto.exe

RAYLIB_FLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(RAYLIB_FLAGS)

run:
	./$(OUT)

clean:
	del /Q bin\JuegoProyecto.exe