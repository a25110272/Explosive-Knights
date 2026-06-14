# Directorios de origen y destino
SRC_DIR := src
BIN_DIR := bin

# Rutas explícitas de MSYS2 (MinGW64) para encontrar SFML y Box2D
INCLUDE_PATHS := -Iinclude -IC:/msys64/mingw64/include
LIB_PATHS := -LC:/msys64/mingw64/lib

# Librerías a enlazar (el orden importa: Box2D al final es una buena práctica)
LIBS := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lbox2d

# Obtener todos los archivos .cpp en el directorio de origen
CPP_FILES := $(filter-out $(SRC_DIR)/Knight.cpp $(SRC_DIR)/Mapa.cpp,$(wildcard $(SRC_DIR)/*.cpp))

# Generar los nombres de los archivos .exe en el directorio de destino
EXE_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.exe,$(CPP_FILES))

# Regla por defecto para compilar todos los archivos .cpp
all: $(EXE_FILES)

# Regla para compilar cada archivo .cpp y generar el archivo .exe correspondiente
$(BIN_DIR)/%.exe: $(SRC_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	g++ $< -o $@ $(INCLUDE_PATHS) $(LIB_PATHS) $(LIBS)

# Regla para ejecutar un archivo específico (ej: make run03_ExplosiveKnights)
run%: $(BIN_DIR)/%.exe
	./$<

# Regla para limpiar los archivos generados
clean:
	rm -f $(BIN_DIR)/*.exe

.PHONY: all clean
