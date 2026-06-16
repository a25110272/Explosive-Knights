#include "Mapa.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>

Mapa::Mapa()
    : objetosMapaCargados(false), mapaBaseCargado(false), temaObjetosActual(-1)
{
    srand(static_cast<unsigned>(time(0)));

    mapaBaseCargado = texturaMapaBase.loadFromFile("assets/images/Mapa.png");
    if (!mapaBaseCargado)
    {
        std::cout << "Error: no se pudo cargar assets/images/Mapa.png" << std::endl;
    }

    sf::Image imagenObjetosMapa;
    objetosMapaCargados = imagenObjetosMapa.loadFromFile("assets/images/Objetos del mapa.png");
    if (objetosMapaCargados)
    {
        texturaObjetosMapa.loadFromImage(imagenObjetosMapa);
        calcularRectsObjetos(imagenObjetosMapa);
    }
    else
    {
        std::cout << "Error: no se pudo cargar assets/images/Objetos del mapa.png" << std::endl;
    }

    inicializarGrid();
}

Mapa::~Mapa()
{
    limpiarFisicas();
}

bool Mapa::cargarFondo(const std::string& ruta)
{
    mapaBaseCargado = texturaMapaBase.loadFromFile(ruta);
    if (!mapaBaseCargado)
    {
        std::cout << "Error: no se pudo cargar " << ruta << std::endl;
    }

    return mapaBaseCargado;
}

void Mapa::inicializarGrid()
{
    bloquesEnDestruccion.clear();

    for (int i = 0; i < 13; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            if (i == 0 || i == 12 || j == 0 || j == 14)
            {
                grid[i][j] = INDESTRUCTIBLE;
            }
            else if (i % 2 == 0 && j % 2 == 0)
            {
                grid[i][j] = INDESTRUCTIBLE;
            }
            else if (esZonaSeguraSpawn(i, j))
            {
                grid[i][j] = VACIO;
            }
            else
            {
                int aleatorio = rand() % 100;
                grid[i][j] = (aleatorio < 40) ? DESTRUCTIBLE : VACIO;
            }
        }
    }
}

void Mapa::generarFisicas(PhysicsSpace& physics)
{
    limpiarFisicas();

    for (int i = 0; i < 13; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            if (grid[i][j] == INDESTRUCTIBLE || grid[i][j] == DESTRUCTIBLE || grid[i][j] == DESTRUYENDOSE)
            {
                float posX = j * 64.0f + 32.0f;
                float posY = i * 64.0f + 32.0f;
                uint32_t categoria = (grid[i][j] == DESTRUCTIBLE || grid[i][j] == DESTRUYENDOSE)
                    ? COLLISION_DESTRUCTIBLE
                    : COLLISION_INDESTRUCTIBLE;
                b2BodyId bodyId = physics.createStaticBody(posX, posY, WALL_HITBOX_SIZE, WALL_HITBOX_SIZE, categoria);
                bodiesMap[{i, j}] = bodyId;
            }
        }
    }
}

void Mapa::draw(sf::RenderWindow& window)
{
    if (mapaBaseCargado)
    {
        sf::Sprite fondo;
        sf::Vector2u size = texturaMapaBase.getSize();
        fondo.setTexture(texturaMapaBase);
        fondo.setScale(960.0f / size.x, 832.0f / size.y);
        fondo.setPosition(0.0f, 0.0f);
        window.draw(fondo);
    }

    for (int i = 0; i < 13; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            if (grid[i][j] != VACIO)
            {
                bool esBorde = (i == 0 || i == 12 || j == 0 || j == 14);
                if (mapaBaseCargado && esBorde)
                {
                    continue;
                }

                if (objetosMapaCargados)
                {
                    dibujarObjetoMapa(window, i, j, grid[i][j]);
                    continue;
                }

                sf::RectangleShape celda(sf::Vector2f(64.0f, 64.0f));
                if (grid[i][j] == INDESTRUCTIBLE)
                {
                    celda.setFillColor(sf::Color(80, 80, 80));
                }
                else if (grid[i][j] == DESTRUCTIBLE || grid[i][j] == DESTRUYENDOSE)
                {
                    celda.setFillColor(sf::Color(139, 90, 43));
                }

                celda.setPosition(j * 64.0f, i * 64.0f);
                window.draw(celda);
            }
        }
    }
}

int Mapa::obtenerTipoCelda(int fila, int columna) const
{
    if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
    {
        return INDESTRUCTIBLE;
    }

    return grid[fila][columna];
}

void Mapa::destruirBloque(int fila, int columna, std::vector<PowerUp>* pItems)
{
    if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
    {
        return;
    }

    if (grid[fila][columna] == DESTRUCTIBLE)
    {
        grid[fila][columna] = DESTRUYENDOSE;
        bloquesEnDestruccion.push_back({fila, columna, DURACION_DESTRUCCION_BLOQUE, pItems});
    }
}

bool Mapa::colocarBloqueIndestructible(int fila, int columna, PhysicsSpace& physics)
{
    if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
    {
        return false;
    }

    if (grid[fila][columna] == INDESTRUCTIBLE)
    {
        return false;
    }

    auto cuerpo = bodiesMap.find({fila, columna});
    if (cuerpo != bodiesMap.end())
    {
        b2BodyId bodyId = cuerpo->second;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
        bodiesMap.erase(cuerpo);
    }

    auto bloque = bloquesEnDestruccion.begin();
    while (bloque != bloquesEnDestruccion.end())
    {
        if (bloque->fila == fila && bloque->columna == columna)
        {
            bloque = bloquesEnDestruccion.erase(bloque);
        }
        else
        {
            ++bloque;
        }
    }

    grid[fila][columna] = INDESTRUCTIBLE;

    float posX = columna * 64.0f + 32.0f;
    float posY = fila * 64.0f + 32.0f;
    b2BodyId bodyId = physics.createStaticBody(posX, posY, WALL_HITBOX_SIZE, WALL_HITBOX_SIZE, COLLISION_INDESTRUCTIBLE);
    bodiesMap[{fila, columna}] = bodyId;

    return true;
}

bool Mapa::colocarBloqueDestructible(int fila, int columna, PhysicsSpace& physics)
{
    if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
    {
        return false;
    }

    if (grid[fila][columna] == INDESTRUCTIBLE)
    {
        return false;
    }

    auto cuerpo = bodiesMap.find({fila, columna});
    if (cuerpo != bodiesMap.end())
    {
        b2BodyId bodyId = cuerpo->second;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
        bodiesMap.erase(cuerpo);
    }

    grid[fila][columna] = DESTRUCTIBLE;

    float posX = columna * 64.0f + 32.0f;
    float posY = fila * 64.0f + 32.0f;
    b2BodyId bodyId = physics.createStaticBody(posX, posY, WALL_HITBOX_SIZE, WALL_HITBOX_SIZE, COLLISION_DESTRUCTIBLE);
    bodiesMap[{fila, columna}] = bodyId;

    return true;
}

void Mapa::limpiarBloquesDestructibles(PhysicsSpace& physics)
{
    (void)physics;

    bloquesEnDestruccion.clear();
    for (int fila = 1; fila < 12; fila++)
    {
        for (int columna = 1; columna < 14; columna++)
        {
            if (grid[fila][columna] != DESTRUCTIBLE && grid[fila][columna] != DESTRUYENDOSE)
            {
                continue;
            }

            grid[fila][columna] = VACIO;
            auto cuerpo = bodiesMap.find({fila, columna});
            if (cuerpo != bodiesMap.end())
            {
                b2BodyId bodyId = cuerpo->second;
                if (b2Body_IsValid(bodyId))
                {
                    b2DestroyBody(bodyId);
                }
                bodiesMap.erase(cuerpo);
            }
        }
    }
}

void Mapa::setTemaObjetos(int tema)
{
    temaObjetosActual = tema;
}

void Mapa::update(float deltaTime)
{
    auto bloque = bloquesEnDestruccion.begin();
    while (bloque != bloquesEnDestruccion.end())
    {
        bloque->tiempoRestante -= deltaTime;
        if (bloque->tiempoRestante > 0.0f)
        {
            ++bloque;
            continue;
        }

        grid[bloque->fila][bloque->columna] = VACIO;

        if (bloque->pItems != nullptr)
        {
            for (auto& item : *bloque->pItems)
            {
                if (item.estaOculto() && item.estaEnCelda(bloque->fila, bloque->columna))
                {
                    item.revelar();
                    break;
                }
            }
        }

        auto cuerpo = bodiesMap.find({bloque->fila, bloque->columna});
        if (cuerpo != bodiesMap.end())
        {
            b2BodyId bodyId = cuerpo->second;
            if (b2Body_IsValid(bodyId))
            {
                b2DestroyBody(bodyId);
            }
            bodiesMap.erase(cuerpo);
        }

        bloque = bloquesEnDestruccion.erase(bloque);
    }
}

int Mapa::getCellType(int fila, int col) const
{
    if (fila >= 0 && fila < 13 && col >= 0 && col < 15)
    {
        return grid[fila][col];
    }

    return INDESTRUCTIBLE;
}

void Mapa::limpiarFisicas()
{
    for (auto& par : bodiesMap)
    {
        b2BodyId bodyId = par.second;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
    }

    bodiesMap.clear();
    bloquesEnDestruccion.clear();
}

bool Mapa::esZonaSeguraSpawn(int fila, int columna) const
{
    return
        (fila == 1 && columna == 1) ||
        (fila == 1 && columna == 2) ||
        (fila == 2 && columna == 1) ||
        (fila == 1 && columna == 13) ||
        (fila == 1 && columna == 12) ||
        (fila == 2 && columna == 13) ||
        (fila == 11 && columna == 1) ||
        (fila == 10 && columna == 1) ||
        (fila == 11 && columna == 2) ||
        (fila == 11 && columna == 13) ||
        (fila == 10 && columna == 13) ||
        (fila == 11 && columna == 12);
}

void Mapa::calcularRectsObjetos(const sf::Image& imagen)
{
    (void)imagen;
    const int columnas = 9;
    const int filas = 4;
    const int rects[filas][columnas][4] = {
        {
            {0, 130, 139, 183}, {139, 174, 107, 139}, {279, 188, 115, 125},
            {422, 179, 116, 134}, {564, 189, 126, 124}, {717, 149, 117, 164},
            {863, 174, 97, 139}, {988, 203, 117, 110}, {1127, 161, 113, 152}
        },
        {
            {0, 407, 139, 205}, {139, 449, 107, 141}, {279, 461, 116, 133},
            {422, 448, 119, 146}, {564, 461, 126, 132}, {718, 396, 113, 200},
            {861, 440, 100, 147}, {988, 454, 116, 134}, {1124, 428, 109, 163}
        },
        {
            {0, 668, 139, 271}, {139, 690, 139, 162}, {278, 703, 117, 147},
            {421, 697, 119, 152}, {563, 707, 127, 142}, {716, 661, 118, 278},
            {860, 690, 102, 164}, {986, 701, 117, 145}, {1119, 673, 122, 266}
        },
        {
            {8, 939, 131, 188}, {139, 946, 139, 157}, {278, 954, 114, 148},
            {418, 948, 120, 152}, {563, 961, 128, 142}, {712, 939, 122, 166},
            {834, 940, 127, 165}, {989, 951, 114, 148}, {1121, 939, 113, 169}
        }
    };

    rectsObjetos.clear();
    rectsObjetos.resize(columnas * filas);

    for (int fila = 0; fila < filas; fila++)
    {
        for (int columna = 0; columna < columnas; columna++)
        {
            rectsObjetos[fila * columnas + columna] = sf::IntRect(
                rects[fila][columna][0],
                rects[fila][columna][1],
                rects[fila][columna][2],
                rects[fila][columna][3]
            );
        }
    }
}

int Mapa::obtenerFilaTema(int fila, int columna) const
{
    if (temaObjetosActual >= 0 && temaObjetosActual < 4)
    {
        return temaObjetosActual;
    }

    if (fila < 6 && columna < 7)
        return 0;
    if (fila < 6)
        return 1;
    if (columna < 7)
        return 3;
    return 2;
}

int Mapa::obtenerColumnaObjeto(int fila, int columna, int tipo) const
{
    if (tipo == DESTRUCTIBLE || tipo == DESTRUYENDOSE)
    {
        const int opcionesDestructibles[5] = {1, 2, 4, 7, 8};
        return opcionesDestructibles[(fila * 3 + columna * 5) % 5];
    }

    return 3;
}

void Mapa::dibujarObjetoMapa(sf::RenderWindow& window, int fila, int columna, int tipo)
{
    const int columnas = 9;
    int filaTema = obtenerFilaTema(fila, columna);
    int columnaObjeto = obtenerColumnaObjeto(fila, columna, tipo);
    int indice = filaTema * columnas + columnaObjeto;

    if (indice < 0 || indice >= static_cast<int>(rectsObjetos.size()))
        return;

    sf::IntRect rect = rectsObjetos[indice];
    sf::Sprite objeto;
    objeto.setTexture(texturaObjetosMapa);
    objeto.setTextureRect(rect);
    objeto.setOrigin(rect.width / 2.0f, rect.height / 2.0f);
    objeto.setScale(MAP_CELL_SIZE / rect.width, MAP_CELL_SIZE / rect.height);
    objeto.setPosition(columna * 64.0f + 32.0f, fila * 64.0f + 32.0f);
    window.draw(objeto);
}
