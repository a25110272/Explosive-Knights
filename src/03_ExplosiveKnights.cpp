#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <box2d/box2d.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>

enum Direccion
{
    ABAJO,
    ARRIBA,
    DERECHA,
    IZQUIERDA
};

// Estados del juego (FASE 5)
enum EstadoJuego
{
    MENU,
    SELECCION_PERSONAJE,
    JUGANDO,
    GAME_OVER,
    VICTORIA
};

enum ModoJuego
{
    ARCADE,
    MULTIJUGADOR
};

// ============== PHYSICS ENGINE (Box2D v3.0) ==============
const float PIXELS_PER_METER = 30.0f;
const float MAP_CELL_SIZE = 64.0f;
const float WALL_HITBOX_SIZE = 56.0f;
const float KNIGHT_HITBOX_SIZE = 34.0f;
const float ENEMY_HITBOX_SIZE = 36.0f;

class PhysicsSpace
{
public:
    PhysicsSpace()
    {
        // Crear mundo con configuración por defecto
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 0.0f};  // Sin gravedad para vista top-down
        
        world = b2CreateWorld(&worldDef);
    }

    ~PhysicsSpace()
    {
        if (b2World_IsValid(world))
        {
            b2DestroyWorld(world);
        }
    }

    b2WorldId getWorldId()
    {
        return world;
    }

    void step(float timeStep)
    {
        if (b2World_IsValid(world))
        {
            b2World_Step(world, timeStep, 4);  // 4 sub-steps
        }
    }

    b2BodyId createDynamicBody(float x, float y, float width, float height)
    {
        if (!b2World_IsValid(world))
        {
            return b2_nullBodyId;
        }

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.x = x / PIXELS_PER_METER;
        bodyDef.position.y = y / PIXELS_PER_METER;
        bodyDef.fixedRotation = true;  // No rotar

        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        // Crear forma de caja
        b2Polygon polygon = b2MakeBox(
            (width / 2.0f) / PIXELS_PER_METER,
            (height / 2.0f) / PIXELS_PER_METER
        );

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.friction = 0.0f;

        b2CreatePolygonShape(bodyId, &shapeDef, &polygon);

        return bodyId;
    }

    b2BodyId createStaticBody(float x, float y, float width, float height)
    {
        if (!b2World_IsValid(world))
        {
            return b2_nullBodyId;
        }

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position.x = x / PIXELS_PER_METER;
        bodyDef.position.y = y / PIXELS_PER_METER;

        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        b2Polygon polygon = b2MakeBox(
            (width / 2.0f) / PIXELS_PER_METER,
            (height / 2.0f) / PIXELS_PER_METER
        );

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 0.0f;

        b2CreatePolygonShape(bodyId, &shapeDef, &polygon);

        return bodyId;
    }

private:
    b2WorldId world;
};

// ============== FIN PHYSICS ENGINE (Box2D v3.0) ==============

// Forward declaration
class PowerUp;

class Personaje
{
public:
    Personaje(sf::Vector2f position, const std::string& texturePath = "assets/images/verde_spritesheet.png")
    {
        if (!textura.loadFromFile("assets/images/verde_spritesheet.png"))
        {
            std::cout << "Error: no se pudo cargar assets/images/verde_spritesheet.png" << std::endl;
        }

        direccion = ABAJO;

        // CAMBIA ESTOS VALORES SEGÚN EL TAMAÑO DE CADA FRAME
        // Ejemplo: si tu imagen mide 1610 x 840:
        // frameWidth = 1610 / 7 = 230
        // frameHeight = 840 / 4 = 210
        frameWidth = 230;
        frameHeight = 235;

        // 7 frames por dirección:
        // frame 0 = quieto
        // frames 1 a 6 = caminando
        numFrames = 7;

        sprite.setTexture(textura);

        sprite.setTextureRect(sf::IntRect(
            0,
            0,
            frameWidth,
            frameHeight
        ));

        centrarSprite();

        sprite.setPosition(position);

        // Cambia esto si se ve muy grande o muy pequeño
        sprite.setScale(0.25f, 0.25f);
        cargarTextura(texturePath);
    }

    bool cargarTextura(const std::string& texturePath)
    {
        sf::Image imagen;
        if (!imagen.loadFromFile(texturePath))
        {
            std::cout << "Error: no se pudo cargar " << texturePath << std::endl;
            return false;
        }

        textura.loadFromImage(imagen);

        sf::Vector2u size = textura.getSize();
        frameWidth = static_cast<int>(size.x / numFrames);
        frameHeight = static_cast<int>(size.y / 4);
        currentFrame = 0;
        calcularOrigenes(imagen);

        sprite.setTexture(textura, true);
        sprite.setTextureRect(sf::IntRect(0, 0, frameWidth, frameHeight));
        sprite.setScale(56.0f / frameHeight, 56.0f / frameHeight);
        centrarSprite();
        return true;
    }

    void move(float offsetX, float offsetY, Direccion nuevaDireccion)
    {
        sprite.move(offsetX, offsetY);
        direccion = nuevaDireccion;
        caminando = true;
    }

    void setPhysicsPosition(float x, float y)
    {
        sprite.setPosition(x, y);
    }

    void detenerAnimacion(Direccion dir)
    {
        // Detiene la animación y muestra el frame de reposo (frame 0)
        direccion = dir;
        currentFrame = 0;
        caminando = false;
        caminandoAnterior = false;
        clock.restart();

        // Actualizar TextureRect al frame 0 de la dirección especificada
        int fila = obtenerFilaDireccion();
        sprite.setTextureRect(sf::IntRect(
            0,  // Frame 0 (reposo)
            fila * frameHeight,
            frameWidth,
            frameHeight
        ));

        centrarSprite();
    }

    void update()
    {
        bool estabaCaminando = caminandoAnterior;

        if (caminando)
        {
            // Si acaba de empezar a caminar, entra directo al frame 1
            // para no pasar por el frame quieto.
            if (!estabaCaminando)
            {
                currentFrame = 1;
                clock.restart();
            }

            if (clock.getElapsedTime().asSeconds() >= frameTime)
            {
                currentFrame++;

                // Caminando solo usa frames 1, 2, 3, 4, 5, 6.
                // Nunca usa el frame 0 mientras se mueve.
                if (currentFrame >= numFrames)
                {
                    currentFrame = 1;
                }

                clock.restart();
            }
        }
        else
        {
            // Si no se mueve, usa frame quieto.
            currentFrame = 0;
            clock.restart();
        }

        int fila = obtenerFilaDireccion();

        sprite.setTextureRect(sf::IntRect(
            currentFrame * frameWidth,
            fila * frameHeight,
            frameWidth,
            frameHeight
        ));

        centrarSprite();

        caminandoAnterior = caminando;
        caminando = false;
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(sprite);
    }

private:
    sf::Sprite sprite;
    sf::Texture textura;
    sf::Clock clock;

    Direccion direccion;

    bool caminando = false;
    bool caminandoAnterior = false;

    int currentFrame = 0;
    int numFrames = 7;

    int frameWidth = 240;
    int frameHeight = 233;
    std::vector<sf::Vector2f> origenesFrames;

    // VELOCIDAD DE ANIMACIÓN
    // Menor número = más rápido
    // Mayor número = más lento
    float frameTime = 0.10f;

   int obtenerFilaDireccion()
{
    if (direccion == ABAJO)
    {
        return 0; // frente
    }
    else if (direccion == ARRIBA)
    {
        return 1; // espalda
    }
    else if (direccion == DERECHA)
    {
        return 2; // derecha
    }
    else
    {
        return 3; // izquierda
    }
}

    void calcularOrigenes(const sf::Image& imagen)
    {
        origenesFrames.clear();
        origenesFrames.resize(numFrames * 4, sf::Vector2f(frameWidth / 2.0f, frameHeight / 2.0f));

        for (int fila = 0; fila < 4; fila++)
        {
            for (int frame = 0; frame < numFrames; frame++)
            {
                int startX = frame * frameWidth;
                int startY = fila * frameHeight;
                int minX = frameWidth;
                int minY = frameHeight;
                int maxX = -1;
                int maxY = -1;

                for (int y = 0; y < frameHeight; y++)
                {
                    for (int x = 0; x < frameWidth; x++)
                    {
                        sf::Color pixel = imagen.getPixel(startX + x, startY + y);
                        if (pixel.a > 10)
                        {
                            if (x < minX) minX = x;
                            if (y < minY) minY = y;
                            if (x > maxX) maxX = x;
                            if (y > maxY) maxY = y;
                        }
                    }
                }

                if (maxX >= minX && maxY >= minY)
                {
                    origenesFrames[fila * numFrames + frame] = sf::Vector2f(
                        (minX + maxX) / 2.0f,
                        (minY + maxY) / 2.0f
                    );
                }
            }
        }
    }

    void centrarSprite()
    {
        int fila = obtenerFilaDireccion();
        int indice = fila * numFrames + currentFrame;
        if (indice >= 0 && indice < static_cast<int>(origenesFrames.size()))
        {
            sprite.setOrigin(origenesFrames[indice]);
        }
        else
        {
            sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
        }
    }
};

// ============== POWER-UP ==============
class PowerUp
{
public:
    PowerUp(int fila, int columna)
        : fila(fila), columna(columna), activo(true)
    {
        // Generar tipo aleatorio (0 = Bota, 1 = Fuego, 2 = Bomba Extra)
        tipo = rand() % 3;
    }

    void draw(sf::RenderWindow& window)
    {
        if (activo)
        {
            float posX = columna * 64.0f + 32.0f;
            float posY = fila * 64.0f + 32.0f;

            sf::CircleShape item(15.0f);
            item.setPosition(posX - 15.0f, posY - 15.0f);

            // Color según tipo
            if (tipo == 0)
            {
                item.setFillColor(sf::Color::Blue);       // Bota
            }
            else if (tipo == 1)
            {
                item.setFillColor(sf::Color::Red);        // Fuego
            }
            else
            {
                item.setFillColor(sf::Color::White);      // Bomba Extra
            }

            window.draw(item);
        }
    }

    int getFila() const { return fila; }
    int getColumna() const { return columna; }
    int getTipo() const { return tipo; }
    bool isActivo() const { return activo; }
    void desactivar() { activo = false; }

private:
    int fila;
    int columna;
    int tipo;  // 0 = Bota, 1 = Fuego, 2 = Bomba Extra
    bool activo;
};

// ============== FIN POWER-UP ==============

// ============== MAPA (FASE 1: Escenario) ==============
class Mapa
{
public:
    // Tipos de celda
    static const int VACIO = 0;
    static const int INDESTRUCTIBLE = 1;
    static const int DESTRUCTIBLE = 2;

    Mapa()
        : objetosMapaCargados(false), mapaBaseCargado(false)
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

    void inicializarGrid()
    {
        const std::string layout[13] = {
            "111111111111111",
            "100022120220001",
            "101010101010101",
            "102012222210201",
            "101010101010101",
            "122220000222221",
            "101000000000101",
            "122220000222221",
            "101010101010101",
            "102012222210201",
            "101010101010101",
            "100022120220001",
            "111111111111111"
        };

        for (int i = 0; i < 13; i++)
        {
            for (int j = 0; j < 15; j++)
            {
                char tile = layout[i][j];
                grid[i][j] = (tile == '1') ? INDESTRUCTIBLE : (tile == '2') ? DESTRUCTIBLE : VACIO;
            }
        }
    }

    void generarFisicas(PhysicsSpace& physics)
    {
        limpiarFisicas();
        // Recorrer grid y crear cuerpos estáticos para muros
        for (int i = 0; i < 13; i++)
        {
            for (int j = 0; j < 15; j++)
            {
                if (grid[i][j] == INDESTRUCTIBLE || grid[i][j] == DESTRUCTIBLE)
                {
                    // Posición en píxeles: centro de la celda
                    float posX = j * 64.0f + 32.0f;
                    float posY = i * 64.0f + 32.0f;

                    // Crear cuerpo estático en Box2D
                    b2BodyId bodyId = physics.createStaticBody(posX, posY, WALL_HITBOX_SIZE, WALL_HITBOX_SIZE);
                    
                    // Guardar el ID en el map (fila, columna) -> b2BodyId
                    bodiesMap[{i, j}] = bodyId;
                }
            }
        }
    }

    void limpiarFisicas()
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
    }

    void draw(sf::RenderWindow& window)
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

                    // Color según tipo
                    if (grid[i][j] == INDESTRUCTIBLE)
                    {
                        celda.setFillColor(sf::Color(80, 80, 80));  // Gris oscuro
                    }
                    else if (grid[i][j] == DESTRUCTIBLE)
                    {
                        celda.setFillColor(sf::Color(139, 90, 43));  // Marrón/Ladrillo
                    }

                    celda.setPosition(j * 64.0f, i * 64.0f);
                    window.draw(celda);
                }
            }
        }
    }

    int obtenerTipoCelda(int fila, int columna) const
    {
        if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
        {
            return INDESTRUCTIBLE;  // Fuera de límites = muro
        }
        return grid[fila][columna];
    }

    void destruirBloque(int fila, int columna, std::vector<PowerUp>* pItems = nullptr)
    {
        // Verificar límites
        if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
        {
            return;
        }

        // Solo destruir bloques DESTRUCTIBLES
        if (grid[fila][columna] == DESTRUCTIBLE)
        {
            grid[fila][columna] = VACIO;

            // Generar item con 30% de probabilidad
            if (pItems != nullptr)
            {
                int aleatorio = rand() % 100;
                if (aleatorio < 30)
                {
                    PowerUp nuevoItem(fila, columna);
                    pItems->push_back(nuevoItem);
                }
            }

            // Buscar y destruir el cuerpo físico en Box2D
            auto it = bodiesMap.find({fila, columna});
            if (it != bodiesMap.end())
            {
                b2BodyId bodyId = it->second;
                if (b2Body_IsValid(bodyId))
                {
                    b2DestroyBody(bodyId);
                }
                bodiesMap.erase(it);
            }
        }
    }

    // Getter para consultar el tipo de celda (FASE 6)
    int getCellType(int fila, int col) const
    {
        if (fila >= 0 && fila < 13 && col >= 0 && col < 15)
            return grid[fila][col];
        return 1;  // Retorna INDESTRUCTIBLE si está fuera de rango
    }

private:
    void calcularRectsObjetos(const sf::Image& imagen)
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

    int obtenerFilaTema(int fila, int columna) const
    {
        if (fila < 6 && columna < 7)
            return 0; // fuego
        if (fila < 6)
            return 1; // rayo/azul
        if (columna < 7)
            return 3; // sombra/morado
        return 2;     // naturaleza/verde
    }

    int obtenerColumnaObjeto(int fila, int columna, int tipo) const
    {
        if (tipo == DESTRUCTIBLE)
        {
            const int opcionesDestructibles[5] = {1, 2, 4, 7, 8};
            return opcionesDestructibles[(fila * 3 + columna * 5) % 5];
        }

        return 3; // bloque de piedra indestructible
    }

    void dibujarObjetoMapa(sf::RenderWindow& window, int fila, int columna, int tipo)
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

    int grid[13][15];
    std::map<std::pair<int, int>, b2BodyId> bodiesMap;  // Mapeo (fila, col) -> b2BodyId
    sf::Texture texturaMapaBase;
    sf::Texture texturaObjetosMapa;
    std::vector<sf::IntRect> rectsObjetos;
    bool mapaBaseCargado;
    bool objetosMapaCargados;
};

// ============== FIN MAPA ==============

// ============== EXPLOSIÓN (FASE 3: Explosiones y Destrucción) ==============
class Explosion
{
public:
    Explosion(int centroFila, int centroCol, Mapa& mapa, int rango = 2, std::vector<PowerUp>* pItems = nullptr,
              sf::Color colorExplosion = sf::Color(255, 165, 0))
        : activa(true), centroFila(centroFila), centroCol(centroCol), color(colorExplosion), pListaItems(pItems)
    {
        // Añadir el centro
        celdasAfectadas.push_back({centroFila, centroCol});

        // 4 direcciones: Arriba (fila-1), Abajo (fila+1), Izquierda (col-1), Derecha (col+1)
        // Arriba
        for (int i = 1; i <= rango; i++)
        {
            int fila = centroFila - i;
            int col = centroCol;

            if (fila < 0 || fila >= 13)
                break;

            int tipo = mapa.obtenerTipoCelda(fila, col);
            if (tipo == Mapa::INDESTRUCTIBLE)
                break;  // El fuego no pasa

            if (tipo == Mapa::DESTRUCTIBLE)
            {
                mapa.destruirBloque(fila, col, pListaItems);
                celdasAfectadas.push_back({fila, col});
                break;  // Destruye pero no sigue
            }

            if (tipo == Mapa::VACIO)
            {
                celdasAfectadas.push_back({fila, col});
            }
        }

        // Abajo
        for (int i = 1; i <= rango; i++)
        {
            int fila = centroFila + i;
            int col = centroCol;

            if (fila < 0 || fila >= 13)
                break;

            int tipo = mapa.obtenerTipoCelda(fila, col);
            if (tipo == Mapa::INDESTRUCTIBLE)
                break;

            if (tipo == Mapa::DESTRUCTIBLE)
            {
                mapa.destruirBloque(fila, col, pListaItems);
                celdasAfectadas.push_back({fila, col});
                break;
            }

            if (tipo == Mapa::VACIO)
            {
                celdasAfectadas.push_back({fila, col});
            }
        }

        // Izquierda
        for (int i = 1; i <= rango; i++)
        {
            int fila = centroFila;
            int col = centroCol - i;

            if (col < 0 || col >= 15)
                break;

            int tipo = mapa.obtenerTipoCelda(fila, col);
            if (tipo == Mapa::INDESTRUCTIBLE)
                break;

            if (tipo == Mapa::DESTRUCTIBLE)
            {
                mapa.destruirBloque(fila, col, pListaItems);
                celdasAfectadas.push_back({fila, col});
                break;
            }

            if (tipo == Mapa::VACIO)
            {
                celdasAfectadas.push_back({fila, col});
            }
        }

        // Derecha
        for (int i = 1; i <= rango; i++)
        {
            int fila = centroFila;
            int col = centroCol + i;

            if (col < 0 || col >= 15)
                break;

            int tipo = mapa.obtenerTipoCelda(fila, col);
            if (tipo == Mapa::INDESTRUCTIBLE)
                break;

            if (tipo == Mapa::DESTRUCTIBLE)
            {
                mapa.destruirBloque(fila, col, pListaItems);
                celdasAfectadas.push_back({fila, col});
                break;
            }

            if (tipo == Mapa::VACIO)
            {
                celdasAfectadas.push_back({fila, col});
            }
        }

        temporizador.restart();
    }

    void update()
    {
        if (activa && temporizador.getElapsedTime().asSeconds() >= 0.5f)
        {
            activa = false;
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (activa)
        {
            for (const auto& celda : celdasAfectadas)
            {
                float posX = celda.y * 64.0f + 32.0f;
                float posY = celda.x * 64.0f + 32.0f;
                bool esCentro = (celda.x == centroFila && celda.y == centroCol);
                bool horizontal = (celda.x == centroFila);

                sf::RectangleShape fuego;
                if (esCentro)
                {
                    fuego.setSize(sf::Vector2f(42.0f, 42.0f));
                }
                else if (horizontal)
                {
                    fuego.setSize(sf::Vector2f(64.0f, 24.0f));
                }
                else
                {
                    fuego.setSize(sf::Vector2f(24.0f, 64.0f));
                }

                fuego.setOrigin(fuego.getSize().x / 2.0f, fuego.getSize().y / 2.0f);
                fuego.setFillColor(color);
                fuego.setPosition(posX, posY);
                window.draw(fuego);

                if (esCentro)
                {
                    sf::CircleShape nucleo(18.0f);
                    nucleo.setOrigin(18.0f, 18.0f);
                    nucleo.setFillColor(sf::Color(color.r, color.g, color.b, 230));
                    nucleo.setPosition(posX, posY);
                    window.draw(nucleo);
                }
            }
        }
    }

    bool isActiva() const { return activa; }

    const std::vector<sf::Vector2i>& getCeldasAfectadas() const { return celdasAfectadas; }

private:
    std::vector<sf::Vector2i> celdasAfectadas;
    sf::Clock temporizador;
    bool activa;
    int centroFila;
    int centroCol;
    sf::Color color;
    std::vector<PowerUp>* pListaItems;
};

// ============== FIN EXPLOSIÓN ==============

// ============== BOMBA (FASE 2: Sistema de Bombas) ==============
class Bomba
{
public:
    Bomba(int fila, int columna, PhysicsSpace& physics, const std::string& texturePath,
          sf::Color colorExplosion, int rango)
        : fila(fila), columna(columna), activa(true), rangoExplosion(rango),
          colorExplosion(colorExplosion), bodyId(b2_nullBodyId), texturaCargada(false)
    {
        // La bomba se maneja por celda y no bloquea fisicamente al jugador.
        (void)physics;
        texturaCargada = textura.loadFromFile(texturePath);
        if (!texturaCargada)
        {
            std::cout << "Error: no se pudo cargar " << texturePath << std::endl;
        }

        // Iniciar temporizador
        temporizador.restart();
    }

    bool update(PhysicsSpace& physics)
    {
        // Verificar si superó los 3 segundos
        if (activa && temporizador.getElapsedTime().asSeconds() >= 3.0f)
        {
            activa = false;
            
            // Destruir cuerpo físico
            if (b2Body_IsValid(bodyId))
            {
                b2DestroyBody(bodyId);
            }

            return true;  // Indicar que explotó este frame
        }

        return false;  // No explotó
    }

    void draw(sf::RenderWindow& window)
    {
        if (activa)
        {
            // Calcular posición en píxeles: centro de la celda
            float posX = columna * 64.0f + 32.0f;
            float posY = fila * 64.0f + 32.0f;

            // Dibujar círculo negro (radio 24px)
            if (texturaCargada)
            {
                sf::Sprite bomba;
                sf::Vector2u size = textura.getSize();
                bomba.setTexture(textura);
                bomba.setTextureRect(sf::IntRect(0, 0, static_cast<int>(size.x), static_cast<int>(size.y)));
                bomba.setOrigin(size.x / 2.0f, size.y / 2.0f);
                float escala = 58.0f / static_cast<float>(std::max(size.x, size.y));
                bomba.setScale(escala, escala);
                bomba.setPosition(posX, posY);
                window.draw(bomba);
            }
            else
            {
                sf::CircleShape bomba(24.0f);
                bomba.setFillColor(sf::Color::Black);
                bomba.setPosition(posX - 24.0f, posY - 24.0f);
                window.draw(bomba);
            }
        }
    }

    bool isActiva() const { return activa; }

    int getFila() const { return fila; }
    int getColumna() const { return columna; }
    int getRangoExplosion() const { return rangoExplosion; }
    sf::Color getColorExplosion() const { return colorExplosion; }

private:
    int fila;
    int columna;
    bool activa;
    int rangoExplosion;
    sf::Color colorExplosion;
    b2BodyId bodyId;
    sf::Clock temporizador;
    sf::Texture textura;
    bool texturaCargada;
};

// ============== FIN BOMBA ==============

// ============== KNIGHT (JUGADOR) - Box2D v3.0 ==============
class Knight
{
public:
    Knight(sf::Vector2f position, PhysicsSpace& physics, const std::string& texturePath = "assets/images/verde_spritesheet.png")
        : personaje(position, texturePath), physicsSpace(physics), 
          maxBombas(1), rangoFuego(1), speed(10.0f),
          speedOriginal(10.0f), vidas(3),
          rutaBomba("assets/images/Bomba verde.png"),
          colorExplosion(sf::Color(90, 255, 90, 190))
    {
        // Crear cuerpo dinámico en Box2D v3.0
        bodyId = physicsSpace.createDynamicBody(
            position.x, 
            position.y, 
            KNIGHT_HITBOX_SIZE,
            KNIGHT_HITBOX_SIZE
        );
        
        posicionOriginal = position;
    }

    void handleInput(sf::Keyboard::Key arriba = sf::Keyboard::W,
                     sf::Keyboard::Key abajo = sf::Keyboard::S,
                     sf::Keyboard::Key derecha = sf::Keyboard::D,
                     sf::Keyboard::Key izquierda = sf::Keyboard::A)
    {
        // Usar la velocidad del Knight (modificable por power-ups)
        
        b2Vec2 velocity = {0.0f, 0.0f};
        Direccion dirActual = direccionActual;
        bool seMovio = false;

        if (sf::Keyboard::isKeyPressed(arriba))
        {
            velocity.y = -speed;
            dirActual = ARRIBA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(abajo))
        {
            velocity.y = speed;
            dirActual = ABAJO;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(derecha))
        {
            velocity.x = speed;
            dirActual = DERECHA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(izquierda))
        {
            velocity.x = -speed;
            dirActual = IZQUIERDA;
            seMovio = true;
        }

        // SOLO actualizar dirección y animación
        // NO mover manualmente (eso lo hace Box2D)
        if (seMovio)
        {
            personaje.move(0, 0, dirActual);
            direccionActual = dirActual;
        }
        else
        {
            // Detener animación: mostrar frame de reposo
            personaje.detenerAnimacion(direccionActual);
        }

        // DELEGAR movimiento 100% a Box2D v3.0
        if (b2Body_IsValid(bodyId))
        {
            b2Body_SetLinearVelocity(bodyId, velocity);
        }
    }

    void syncWithPhysics()
    {
        // Sincronizar posición del sprite con el cuerpo Box2D
        if (b2Body_IsValid(bodyId))
        {
            b2Vec2 pos = b2Body_GetPosition(bodyId);
            float pixelX = pos.x * PIXELS_PER_METER;
            float pixelY = pos.y * PIXELS_PER_METER;
            
            personaje.setPhysicsPosition(pixelX, pixelY);
        }
    }

    void update()
    {
        personaje.update();
    }

    void draw(sf::RenderWindow& window)
    {
        personaje.draw(window);
    }

    void cambiarSprite(const std::string& texturePath)
    {
        personaje.cargarTextura(texturePath);
    }

    void configurarBomba(const std::string& texturePath, sf::Color color)
    {
        rutaBomba = texturePath;
        colorExplosion = color;
    }

    void moverA(float posX, float posY)
    {
        if (b2Body_IsValid(bodyId))
        {
            b2Transform transform = {{posX / PIXELS_PER_METER, posY / PIXELS_PER_METER}, b2Rot_identity};
            b2Body_SetTransform(bodyId, transform.p, transform.q);
            b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
        }
        personaje.setPhysicsPosition(posX, posY);
    }

    void reiniciar(float posX, float posY)
    {
        vidas = 3;
        maxBombas = 1;
        rangoFuego = 1;
        speed = speedOriginal;
        posicionOriginal = sf::Vector2f(posX, posY);
        moverA(posX, posY);
    }

    void plantarBomba(Mapa& mapa, std::vector<Bomba>& bombas, PhysicsSpace& physics)
    {
        // Verificar que no se supere el máximo de bombas
        int bombasActivas = 0;
        for (const auto& bomba : bombas)
        {
            if (bomba.isActiva())
            {
                bombasActivas++;
            }
        }

        if (bombasActivas >= maxBombas)
        {
            return;  // No puede plantar más bombas
        }

        // Obtener posición actual del Knight en píxeles
        b2Vec2 posKnight = b2Body_GetPosition(bodyId);
        float pixelX = posKnight.x * PIXELS_PER_METER;
        float pixelY = posKnight.y * PIXELS_PER_METER;

        // Calcular celda (fila, columna) en el grid
        int columna = static_cast<int>(pixelX / 64.0f);
        int fila = static_cast<int>(pixelY / 64.0f);

        // Validar que la celda esté dentro del mapa
        if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
        {
            return;
        }

        // Verificar que no haya un muro (tipo 1 o 2) en esa celda
        if (mapa.obtenerTipoCelda(fila, columna) != Mapa::VACIO)
        {
            return;
        }

        // Crear nueva bomba
        Bomba nuevaBomba(fila, columna, physics, rutaBomba, colorExplosion, rangoFuego);
        bombas.push_back(nuevaBomba);
    }

    void recolectarItem(int tipo)
    {
        if (tipo == 0)
        {
            // Bota: aumentar velocidad
            speed = speedOriginal * 1.5f;
        }
        else if (tipo == 1)
        {
            // Fuego: aumentar rango
            rangoFuego++;
        }
        else if (tipo == 2)
        {
            // Bomba Extra: aumentar máximo de bombas
            maxBombas++;
        }
    }

    void morir(PhysicsSpace& physics)
    {
        // Restar una vida
        vidas--;

        // Si aún hay vidas, teletransportar al inicio
        if (vidas > 0)
        {
            // Reiniciar posición física al centro de [1][1]
            float posX = posicionOriginal.x;
            float posY = posicionOriginal.y;

            if (b2Body_IsValid(bodyId))
            {
                b2Transform transform = {{posX / PIXELS_PER_METER, posY / PIXELS_PER_METER}, b2Rot_identity};
                b2Body_SetTransform(bodyId, transform.p, transform.q);
                b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
            }

            // Reiniciar stats a valores por defecto
            maxBombas = 1;
            rangoFuego = 1;
            speed = speedOriginal;
        }
    }

    b2BodyId getBodyId() { return bodyId; }
    int getRangoFuego() const { return rangoFuego; }
    int getMaxBombas() const { return maxBombas; }
    int getVidas() const { return vidas; }

private:
    Personaje personaje;
    b2BodyId bodyId;
    PhysicsSpace& physicsSpace;
    Direccion direccionActual = ABAJO;
    int maxBombas;
    int rangoFuego;
    float speed;
    float speedOriginal;
    sf::Vector2f posicionOriginal;
    int vidas;
    std::string rutaBomba;
    sf::Color colorExplosion;
};

// ============== FIN KNIGHT - Box2D v3.0 ==============

// ============== ENEMIGO (FASE 4: Enemigos e IA de Patrullaje) ==============
class Enemigo
{
public:
    Enemigo(float x, float y, PhysicsSpace& physics, const std::string& texturePath)
        : speed(6.0f), vivo(true), dirActual(ABAJO)
    {
        // Crear cuerpo dinámico en Box2D v3.0
        bodyId = physics.createDynamicBody(x, y, ENEMY_HITBOX_SIZE, ENEMY_HITBOX_SIZE);

        // Cargar textura y sprite
        if (!textura.loadFromFile(texturePath))
        {
            std::cerr << "Error: No se pudo cargar textura: " << texturePath << std::endl;
        }

        sprite.setTexture(textura);
        sprite.setScale(0.25f, 0.25f);
        sprite.setPosition(x - 12.0f, y - 12.0f);  // Centrar sprite
    }

    void update(Mapa& mapa, PhysicsSpace& physics)
    {
        if (!vivo)
            return;

        // Aplicar velocidad según dirección actual
        b2Vec2 velocity = {0.0f, 0.0f};

        if (dirActual == ARRIBA)
            velocity.y = -speed;
        else if (dirActual == ABAJO)
            velocity.y = speed;
        else if (dirActual == DERECHA)
            velocity.x = speed;
        else if (dirActual == IZQUIERDA)
            velocity.x = -speed;

        if (b2Body_IsValid(bodyId))
        {
            b2Body_SetLinearVelocity(bodyId, velocity);

            // IA: Si la velocidad es muy baja, intentar cambiar de dirección
            b2Vec2 velActual = b2Body_GetLinearVelocity(bodyId);
            float speedActual = std::sqrt(velActual.x * velActual.x + velActual.y * velActual.y);

            if (speedActual < 0.5f)  // Está atorado
            {
                // Obtener posición en grid
                b2Vec2 pos = b2Body_GetPosition(bodyId);
                float pixelX = pos.x * PIXELS_PER_METER;
                float pixelY = pos.y * PIXELS_PER_METER;
                int filaActual = static_cast<int>(pixelY / 64.0f);
                int colActual = static_cast<int>(pixelX / 64.0f);

                // Evaluar celdas adyacentes
                std::vector<Direccion> direccionesLibres;

                // Arriba
                if (mapa.obtenerTipoCelda(filaActual - 1, colActual) == Mapa::VACIO)
                    direccionesLibres.push_back(ARRIBA);
                // Abajo
                if (mapa.obtenerTipoCelda(filaActual + 1, colActual) == Mapa::VACIO)
                    direccionesLibres.push_back(ABAJO);
                // Derecha
                if (mapa.obtenerTipoCelda(filaActual, colActual + 1) == Mapa::VACIO)
                    direccionesLibres.push_back(DERECHA);
                // Izquierda
                if (mapa.obtenerTipoCelda(filaActual, colActual - 1) == Mapa::VACIO)
                    direccionesLibres.push_back(IZQUIERDA);

                // Cambiar a una dirección libre aleatoria
                if (!direccionesLibres.empty())
                {
                    int idx = rand() % direccionesLibres.size();
                    dirActual = direccionesLibres[idx];
                }
            }
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (!vivo)
            return;

        if (b2Body_IsValid(bodyId))
        {
            b2Vec2 pos = b2Body_GetPosition(bodyId);
            float pixelX = pos.x * PIXELS_PER_METER;
            float pixelY = pos.y * PIXELS_PER_METER;

            sprite.setPosition(pixelX - 12.0f, pixelY - 12.0f);
            window.draw(sprite);
        }
    }

    void destruir(PhysicsSpace& physics)
    {
        vivo = false;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
    }

    bool isVivo() const { return vivo; }

    b2BodyId getBodyId() { return bodyId; }

private:
    b2BodyId bodyId;
    sf::Sprite sprite;
    sf::Texture textura;
    Direccion dirActual;
    float speed;
    bool vivo;
};

// ============== FIN ENEMIGO ==============

// ============== BOSS (FASE 6) ==============
class Boss
{
public:
    Boss(float x, float y, PhysicsSpace& physics, const std::string& texturePath)
        : hp(3), cooldownDano(0), dirActual(Direccion::DERECHA), speed(3.5f)
    {
        // Crear cuerpo físico: DYNAMIC, 96x96 px, sin rotación
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {x / PIXELS_PER_METER, y / PIXELS_PER_METER};
        bodyDef.linearDamping = 0.5f;
        bodyId = b2CreateBody(physics.getWorldId(), &bodyDef);

        // Crear forma: caja de 96x96 px = 3.2m x 3.2m
        b2Polygon polygonShape = b2MakeBox(96.0f / (2.0f * PIXELS_PER_METER), 
                                           96.0f / (2.0f * PIXELS_PER_METER));
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2CreatePolygonShape(bodyId, &shapeDef, &polygonShape);

        // Bloquear rotación
        b2Body_SetFixedRotation(bodyId, true);

        // Cargar textura
        if (!textura.loadFromFile(texturePath))
        {
            std::cerr << "Error: No se pudo cargar textura: " << texturePath << std::endl;
        }

        sprite.setTexture(textura);
        sprite.setScale(0.5f, 0.5f);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, 
                         sprite.getLocalBounds().height / 2.0f);
    }

    void update(Mapa& mapa, PhysicsSpace& physics)
    {
        if (cooldownDano > 0)
            cooldownDano--;

        // Aplicar velocidad según dirección
        float velX = 0.0f, velY = 0.0f;
        switch (dirActual)
        {
        case Direccion::ARRIBA:    velY = -speed; break;
        case Direccion::ABAJO:     velY = speed;  break;
        case Direccion::IZQUIERDA: velX = -speed; break;
        case Direccion::DERECHA:   velX = speed;  break;
        }
        b2Body_SetLinearVelocity(bodyId, {velX, velY});

        // Detectar colisión: si velocidad es ~0, cambiar dirección
        b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
        float velocityMagnitude = std::sqrt(vel.x * vel.x + vel.y * vel.y);

        if (velocityMagnitude < 0.1f)
        {
            // Evaluamos las 4 direcciones adyacentes
            b2Vec2 bosPos = b2Body_GetPosition(bodyId);
            int bosRow = static_cast<int>((bosPos.y * PIXELS_PER_METER) / 64.0f);
            int bosCol = static_cast<int>((bosPos.x * PIXELS_PER_METER) / 64.0f);

            std::vector<Direccion> direccionesLibres;
            
            // Arriba
            if (bosRow > 0 && mapa.getCellType(bosRow - 1, bosCol) != 1)
                direccionesLibres.push_back(Direccion::ARRIBA);
            // Abajo
            if (bosRow < 12 && mapa.getCellType(bosRow + 1, bosCol) != 1)
                direccionesLibres.push_back(Direccion::ABAJO);
            // Izquierda
            if (bosCol > 0 && mapa.getCellType(bosRow, bosCol - 1) != 1)
                direccionesLibres.push_back(Direccion::IZQUIERDA);
            // Derecha
            if (bosCol < 14 && mapa.getCellType(bosRow, bosCol + 1) != 1)
                direccionesLibres.push_back(Direccion::DERECHA);

            if (!direccionesLibres.empty())
            {
                int idx = rand() % direccionesLibres.size();
                dirActual = direccionesLibres[idx];
            }
        }

        // HABILIDAD JUGGERNAUT: Destruir bloques que pisa
        b2Vec2 bosPos = b2Body_GetPosition(bodyId);
        int bosRow = static_cast<int>((bosPos.y * PIXELS_PER_METER) / 64.0f);
        int bosCol = static_cast<int>((bosPos.x * PIXELS_PER_METER) / 64.0f);

        if (bosRow >= 0 && bosRow < 13 && bosCol >= 0 && bosCol < 15)
        {
            if (mapa.getCellType(bosRow, bosCol) == 2)
            {
                mapa.destruirBloque(bosRow, bosCol, nullptr);
            }
        }
    }

    void recibirDano()
    {
        if (cooldownDano <= 0)
        {
            hp--;
            cooldownDano = 60;  // 1 segundo a 60 FPS
        }
    }

    void destruir(PhysicsSpace& physics)
    {
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
    }

    void draw(sf::RenderWindow& window)
    {
        b2Vec2 pos = b2Body_GetPosition(bodyId);
        sprite.setPosition(pos.x * PIXELS_PER_METER, pos.y * PIXELS_PER_METER);

        // Efecto parpadeo si está en cooldown
        if (cooldownDano > 0)
        {
            // Parpadea rojo
            if (cooldownDano % 10 < 5)
                sprite.setColor(sf::Color(255, 100, 100, 255));  // Rojo
            else
                sprite.setColor(sf::Color(255, 255, 255, 255));  // Normal
        }
        else
        {
            sprite.setColor(sf::Color(255, 255, 255, 255));
        }

        window.draw(sprite);
    }

    int getHp() const { return hp; }
    b2BodyId getBodyId() { return bodyId; }

private:
    b2BodyId bodyId;
    sf::Sprite sprite;
    sf::Texture textura;
    int hp;
    int cooldownDano;
    Direccion dirActual;
    float speed;
};

// ============== FIN BOSS ==============

// ============== FUNCIÓN CARGAR NIVEL (FASE 5 y 6) ==============
void cargarNivel(int nivel, Mapa& mapa, Knight& knight, std::vector<Enemigo>& enemigos,
                 std::vector<Bomba>& bombas, std::vector<Explosion>& explosiones,
                 std::vector<PowerUp>& items, std::vector<Boss>& jefes, PhysicsSpace& physics)
{
    // Limpiar listas
    enemigos.clear();
    bombas.clear();
    explosiones.clear();
    items.clear();
    jefes.clear();

    // Regenerar mapa
    mapa.inicializarGrid();
    mapa.generarFisicas(physics);

    // Teletransportar Knight a inicio
    float posX = 1.0f * 64.0f + 32.0f;
    float posY = 1.0f * 64.0f + 32.0f;
    if (b2Body_IsValid(knight.getBodyId()))
    {
        b2Transform transform = {{posX / 30.0f, posY / 30.0f}, b2Rot_identity};
        b2Body_SetTransform(knight.getBodyId(), transform.p, transform.q);
        b2Body_SetLinearVelocity(knight.getBodyId(), {0.0f, 0.0f});
    }

    // FASE 6: Si es nivel 3, spawnear el Boss en lugar de enemigos normales
    if (nivel == 3)
    {
        // Spawneamos el Boss en el centro del mapa [6][7]
        float bosX = 7.0f * 64.0f + 32.0f;
        float bosY = 6.0f * 64.0f + 32.0f;
        jefes.emplace_back(bosX, bosY, physics, "assets/images/JEFE.png");
        std::cout << "NIVEL 3 INICIADO - ¡EL JEFE FINAL HA LLEGADO!" << std::endl;
        return;
    }

    // Spawnear enemigos normales para niveles 1 y 2
    int cantidadEnemigos = 3;
    if (nivel == 2)
        cantidadEnemigos = 5;


    // Configuración de spawn en esquinas (se repite si hay más enemigos)
    std::vector<std::pair<float, float>> spawnPoints = {
        {1.0f * 64.0f + 32.0f, 11.0f * 64.0f + 32.0f},    // [11][1]
        {13.0f * 64.0f + 32.0f, 1.0f * 64.0f + 32.0f},    // [1][13]
        {13.0f * 64.0f + 32.0f, 11.0f * 64.0f + 32.0f},   // [11][13]
        {1.0f * 64.0f + 32.0f, 1.0f * 64.0f + 32.0f},     // [1][1] - cerca del knight
        {7.0f * 64.0f + 32.0f, 6.0f * 64.0f + 32.0f},     // Centro
        {1.0f * 64.0f + 32.0f, 6.0f * 64.0f + 32.0f},     // Izquierda centro
        {13.0f * 64.0f + 32.0f, 6.0f * 64.0f + 32.0f},    // Derecha centro
    };

    for (int i = 0; i < cantidadEnemigos && i < static_cast<int>(spawnPoints.size()); i++)
    {
        enemigos.emplace_back(spawnPoints[i].first, spawnPoints[i].second, physics, "assets/images/ENEMIGO1.png");
    }

    std::cout << "NIVEL " << nivel << " INICIADO - " << cantidadEnemigos << " ENEMIGOS" << std::endl;
}

// ============== FIN FUNCIÓN CARGAR NIVEL ==============

int main()
{
    sf::RenderWindow window(sf::VideoMode(960, 832), "Explosive-Knights - FASE 5");
    window.setFramerateLimit(60);

    // Máquina de estados y nivel (FASE 5)
    EstadoJuego estadoActual = MENU;
    ModoJuego modoActual = ARCADE;
    int nivelActual = 1;
    int modoSeleccionado = 0;
    int caballeroSeleccionado = 0;
    int caballeroSeleccionadoP2 = 1;

    // Instanciar Physics Engine
    PhysicsSpace physics;

    // Instanciar Mapa
    Mapa mapa;

    // Instanciar Knight (jugador)
    Knight knight(sf::Vector2f(96.0f, 96.0f), physics);
    Knight knight2(sf::Vector2f(-500.0f, -500.0f), physics, "assets/images/Azul_caminar.png");

    // Vector de bombas (FASE 2)
    std::vector<Bomba> listaBombas;

    // Vector de explosiones (FASE 3)
    std::vector<Explosion> listaExplosiones;

    // Vector de power-ups
    std::vector<PowerUp> listaItems;

    // Vector de enemigos (FASE 4)
    std::vector<Enemigo> listaEnemigos;

    // Vector de jefes (FASE 6)
    std::vector<Boss> listaJefes;

    std::vector<std::string> nombresModos = {"ARCADE", "VERSUS"};
    std::vector<std::string> nombresCaballeros = {"VERDE", "AZUL", "ROJO", "NEGRO"};
    std::vector<std::string> rutasCaballeros = {
        "assets/images/verde_spritesheet.png",
        "assets/images/Azul_caminar.png",
        "assets/images/Rojo_caminar.png",
        "assets/images/Negro_caminar.png"
    };
    std::vector<std::string> rutasBombas = {
        "assets/images/Bomba verde.png",
        "assets/images/Bomba azul.png",
        "assets/images/Bomba Roja.png",
        "assets/images/Bomba negra.png"
    };
    std::vector<sf::Color> coloresExplosiones = {
        sf::Color(80, 255, 100, 190),
        sf::Color(80, 190, 255, 190),
        sf::Color(255, 80, 70, 190),
        sf::Color(170, 90, 255, 190)
    };

    std::vector<sf::Texture> texturasCaballeros(rutasCaballeros.size());
    for (int i = 0; i < static_cast<int>(rutasCaballeros.size()); i++)
    {
        if (!texturasCaballeros[i].loadFromFile(rutasCaballeros[i]))
        {
            std::cout << "Error: no se pudo cargar " << rutasCaballeros[i] << std::endl;
        }
    }

    sf::Texture texturaPantallaInicio;
    bool pantallaInicioCargada = texturaPantallaInicio.loadFromFile("assets/images/Pantalla de inicio.png");
    if (!pantallaInicioCargada)
    {
        std::cout << "Error: no se pudo cargar assets/images/Pantalla de inicio.png" << std::endl;
    }

    // SISTEMA DE AUDIO (FASE 1)
    sf::Music musicaFondo;
    // Cargar música de fondo (archivo ficticio - modificar cuando exista el archivo real)
    if (!musicaFondo.openFromFile("assets/audio/stage1.ogg"))
    {
        std::cout << "Aviso: No se pudo cargar assets/audio/stage1.ogg" << std::endl;
    }
    else
    {
        musicaFondo.setLoop(true);
        musicaFondo.play();
    }

    // INTERFAZ GRÁFICA (FASE 5)
    sf::Font fuente;
    bool fuenteCargada = false;
    
    // Intentar cargar una fuente del sistema (Windows)
    if (fuente.loadFromFile("C:\\Windows\\Fonts\\arial.ttf"))
    {
        fuenteCargada = true;
    }
    
    sf::Text textoHUD;
    if (fuenteCargada)
    {
        textoHUD.setFont(fuente);
        textoHUD.setCharacterSize(20);
        textoHUD.setFillColor(sf::Color::White);
        textoHUD.setPosition(10.0f, 10.0f);
    }

    sf::Text textoGameOver;
    if (fuenteCargada)
    {
        textoGameOver.setFont(fuente);
        textoGameOver.setCharacterSize(60);
        textoGameOver.setFillColor(sf::Color::Red);
        textoGameOver.setString("GAME OVER");
        // Centrar horizontalmente
        textoGameOver.setPosition(960.0f / 2.0f - textoGameOver.getLocalBounds().width / 2.0f, 832.0f / 2.0f - 60.0f);
    }

    sf::Text textoVictoria;
    if (fuenteCargada)
    {
        textoVictoria.setFont(fuente);
        textoVictoria.setCharacterSize(60);
        textoVictoria.setFillColor(sf::Color::Green);
        textoVictoria.setString("VICTORIA");
        textoVictoria.setPosition(960.0f / 2.0f - textoVictoria.getLocalBounds().width / 2.0f, 832.0f / 2.0f - 60.0f);
    }

    sf::Text textoInstruccion;
    if (fuenteCargada)
    {
        textoInstruccion.setFont(fuente);
        textoInstruccion.setCharacterSize(24);
        textoInstruccion.setFillColor(sf::Color::White);
        textoInstruccion.setString("Presiona ENTER para reiniciar");
        textoInstruccion.setPosition(960.0f / 2.0f - textoInstruccion.getLocalBounds().width / 2.0f, 832.0f / 2.0f + 60.0f);
    }

    const float timeStep = 1.0f / 60.0f;  // 60 FPS

    while (window.isOpen())
    {
        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            // Manejar input de Espacio para plantar bomba (FASE 2)
            if (event.type == sf::Event::KeyPressed)
            {
                if (estadoActual == MENU)
                {
                    if (event.key.code == sf::Keyboard::Up)
                    {
                        modoSeleccionado--;
                        if (modoSeleccionado < 0)
                            modoSeleccionado = 2;
                    }
                    else if (event.key.code == sf::Keyboard::Down)
                    {
                        modoSeleccionado++;
                        if (modoSeleccionado > 2)
                            modoSeleccionado = 0;
                    }
                    else if (event.key.code == sf::Keyboard::Return)
                    {
                        if (modoSeleccionado == 2)
                        {
                            window.close();
                        }
                        else
                        {
                            estadoActual = SELECCION_PERSONAJE;
                        }
                    }
                }
                else if (estadoActual == SELECCION_PERSONAJE)
                {
                    if (event.key.code == sf::Keyboard::Left)
                    {
                        caballeroSeleccionado--;
                        if (caballeroSeleccionado < 0)
                            caballeroSeleccionado = static_cast<int>(rutasCaballeros.size()) - 1;
                    }
                    else if (event.key.code == sf::Keyboard::Right)
                    {
                        caballeroSeleccionado++;
                        if (caballeroSeleccionado >= static_cast<int>(rutasCaballeros.size()))
                            caballeroSeleccionado = 0;
                    }
                    else if (modoSeleccionado == 1 && event.key.code == sf::Keyboard::A)
                    {
                        caballeroSeleccionadoP2--;
                        if (caballeroSeleccionadoP2 < 0)
                            caballeroSeleccionadoP2 = static_cast<int>(rutasCaballeros.size()) - 1;
                    }
                    else if (modoSeleccionado == 1 && event.key.code == sf::Keyboard::D)
                    {
                        caballeroSeleccionadoP2++;
                        if (caballeroSeleccionadoP2 >= static_cast<int>(rutasCaballeros.size()))
                            caballeroSeleccionadoP2 = 0;
                    }
                    else if (event.key.code == sf::Keyboard::Escape)
                    {
                        estadoActual = MENU;
                    }
                    else if (event.key.code == sf::Keyboard::Return)
                    {
                        modoActual = (modoSeleccionado == 0) ? ARCADE : MULTIJUGADOR;
                        knight.cambiarSprite(rutasCaballeros[caballeroSeleccionado]);
                        knight2.cambiarSprite(rutasCaballeros[caballeroSeleccionadoP2]);
                        knight.configurarBomba(rutasBombas[caballeroSeleccionado], coloresExplosiones[caballeroSeleccionado]);
                        knight2.configurarBomba(rutasBombas[caballeroSeleccionadoP2], coloresExplosiones[caballeroSeleccionadoP2]);
                        estadoActual = JUGANDO;
                        nivelActual = 1;

                        if (modoActual == ARCADE)
                        {
                            knight.reiniciar(96.0f, 96.0f);
                            knight2.moverA(-500.0f, -500.0f);
                            cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics);
                        }
                        else
                        {
                            listaEnemigos.clear();
                            listaBombas.clear();
                            listaExplosiones.clear();
                            listaItems.clear();
                            listaJefes.clear();
                            mapa.inicializarGrid();
                            mapa.generarFisicas(physics);
                            knight.reiniciar(96.0f, 96.0f);
                            knight2.reiniciar(864.0f, 736.0f);
                        }
                    }
                }
                else if (event.key.code == sf::Keyboard::Space && estadoActual == JUGANDO)
                {
                    knight.plantarBomba(mapa, listaBombas, physics);
                }
                else if (event.key.code == sf::Keyboard::Return && estadoActual == JUGANDO && modoActual == MULTIJUGADOR)
                {
                    knight2.plantarBomba(mapa, listaBombas, physics);
                }
                // Reiniciar con Enter si es GAME_OVER o VICTORIA (FASE 5)
                else if (event.key.code == sf::Keyboard::Return && (estadoActual == GAME_OVER || estadoActual == VICTORIA))
                {
                    estadoActual = MENU;
                }
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        if (estadoActual == MENU || estadoActual == SELECCION_PERSONAJE)
        {
            window.clear(sf::Color(28, 28, 38));

            if (estadoActual == MENU)
            {
                if (pantallaInicioCargada)
                {
                    sf::Sprite fondoInicio;
                    sf::Vector2u size = texturaPantallaInicio.getSize();
                    float scale = 960.0f / size.x;
                    float scaledHeight = size.y * scale;
                    fondoInicio.setTexture(texturaPantallaInicio);
                    fondoInicio.setScale(scale, scale);
                    fondoInicio.setPosition(0.0f, (832.0f - scaledHeight) / 2.0f);
                    window.draw(fondoInicio);

                    float selectorY[3] = {548.0f, 626.0f, 700.0f};
                    float selectorWidth[3] = {360.0f, 290.0f, 190.0f};
                    float sourceToScreen = scale;
                    float offsetY = (832.0f - scaledHeight) / 2.0f;

                    sf::RectangleShape selector(sf::Vector2f(selectorWidth[modoSeleccionado] * sourceToScreen, 54.0f * sourceToScreen));
                    selector.setOrigin(selector.getSize().x / 2.0f, selector.getSize().y / 2.0f);
                    selector.setPosition(480.0f, offsetY + selectorY[modoSeleccionado] * sourceToScreen);
                    selector.setFillColor(sf::Color(0, 0, 0, 90));
                    selector.setOutlineColor(sf::Color(255, 190, 60));
                    selector.setOutlineThickness(3.0f);
                    window.draw(selector);

                    sf::CircleShape flechaIzq(9.0f, 3);
                    flechaIzq.setOrigin(9.0f, 9.0f);
                    flechaIzq.setFillColor(sf::Color(255, 190, 60));
                    flechaIzq.setRotation(90.0f);
                    flechaIzq.setPosition(selector.getPosition().x - selector.getSize().x / 2.0f - 22.0f, selector.getPosition().y);
                    window.draw(flechaIzq);

                    sf::CircleShape flechaDer(9.0f, 3);
                    flechaDer.setOrigin(9.0f, 9.0f);
                    flechaDer.setFillColor(sf::Color(255, 190, 60));
                    flechaDer.setRotation(-90.0f);
                    flechaDer.setPosition(selector.getPosition().x + selector.getSize().x / 2.0f + 22.0f, selector.getPosition().y);
                    window.draw(flechaDer);
                }
                else if (fuenteCargada)
                {
                    sf::Text aviso;
                    aviso.setFont(fuente);
                    aviso.setCharacterSize(32);
                    aviso.setFillColor(sf::Color::White);
                    aviso.setString("Explosive Knights\nEnter: iniciar");
                    aviso.setPosition(260.0f, 320.0f);
                    window.draw(aviso);
                }
            }
            else if (fuenteCargada)
            {
                sf::Text titulo;
                titulo.setFont(fuente);
                titulo.setCharacterSize(56);
                titulo.setFillColor(sf::Color::White);
                titulo.setString("EXPLOSIVE KNIGHTS");
                titulo.setPosition(960.0f / 2.0f - titulo.getLocalBounds().width / 2.0f, 70.0f);
                window.draw(titulo);

                    sf::Text subtitulo;
                    subtitulo.setFont(fuente);
                    subtitulo.setCharacterSize(30);
                    subtitulo.setFillColor(sf::Color(220, 220, 220));
                    subtitulo.setString("Elige tu caballero");
                    subtitulo.setPosition(960.0f / 2.0f - subtitulo.getLocalBounds().width / 2.0f, 170.0f);
                    window.draw(subtitulo);

                    if (texturasCaballeros[caballeroSeleccionado].getSize().x > 0)
                    {
                        sf::Sprite preview;
                        sf::Vector2u size = texturasCaballeros[caballeroSeleccionado].getSize();
                        int previewFrameWidth = static_cast<int>(size.x / 7);
                        int previewFrameHeight = static_cast<int>(size.y / 4);
                        preview.setTexture(texturasCaballeros[caballeroSeleccionado]);
                        preview.setTextureRect(sf::IntRect(0, 0, previewFrameWidth, previewFrameHeight));
                        preview.setOrigin(previewFrameWidth / 2.0f, previewFrameHeight / 2.0f);
                        preview.setScale(160.0f / previewFrameHeight, 160.0f / previewFrameHeight);
                        preview.setPosition(modoSeleccionado == 1 ? 340.0f : 480.0f, 390.0f);
                        window.draw(preview);
                    }

                    if (modoSeleccionado == 1 && texturasCaballeros[caballeroSeleccionadoP2].getSize().x > 0)
                    {
                        sf::Sprite previewP2;
                        sf::Vector2u size = texturasCaballeros[caballeroSeleccionadoP2].getSize();
                        int previewFrameWidth = static_cast<int>(size.x / 7);
                        int previewFrameHeight = static_cast<int>(size.y / 4);
                        previewP2.setTexture(texturasCaballeros[caballeroSeleccionadoP2]);
                        previewP2.setTextureRect(sf::IntRect(0, 0, previewFrameWidth, previewFrameHeight));
                        previewP2.setOrigin(previewFrameWidth / 2.0f, previewFrameHeight / 2.0f);
                        previewP2.setScale(160.0f / previewFrameHeight, 160.0f / previewFrameHeight);
                        previewP2.setPosition(620.0f, 390.0f);
                        window.draw(previewP2);
                    }

                    sf::Text nombre;
                    nombre.setFont(fuente);
                    nombre.setCharacterSize(40);
                    nombre.setFillColor(sf::Color::Yellow);
                    nombre.setString(modoSeleccionado == 1 ? "P1 < " + nombresCaballeros[caballeroSeleccionado] + " >" : "< " + nombresCaballeros[caballeroSeleccionado] + " >");
                    nombre.setPosition(modoSeleccionado == 1 ? 180.0f : 960.0f / 2.0f - nombre.getLocalBounds().width / 2.0f, 550.0f);
                    window.draw(nombre);

                    if (modoSeleccionado == 1)
                    {
                        sf::Text nombreP2;
                        nombreP2.setFont(fuente);
                        nombreP2.setCharacterSize(40);
                        nombreP2.setFillColor(sf::Color::Cyan);
                        nombreP2.setString("P2 < " + nombresCaballeros[caballeroSeleccionadoP2] + " >");
                        nombreP2.setPosition(520.0f, 550.0f);
                        window.draw(nombreP2);
                    }

                    sf::Text modo;
                    modo.setFont(fuente);
                    modo.setCharacterSize(24);
                    modo.setFillColor(sf::Color(210, 210, 210));
                    modo.setString("Modo: " + nombresModos[modoSeleccionado]);
                    modo.setPosition(960.0f / 2.0f - modo.getLocalBounds().width / 2.0f, 620.0f);
                    window.draw(modo);

                    sf::Text ayuda;
                    ayuda.setFont(fuente);
                    ayuda.setCharacterSize(22);
                    ayuda.setFillColor(sf::Color(180, 180, 180));
                    ayuda.setString(modoSeleccionado == 1 ? "P1: Izq/Der | P2: A/D | Enter: jugar | Esc: volver" : "Izquierda/Derecha: cambiar | Enter: jugar | Esc: volver");
                    ayuda.setPosition(960.0f / 2.0f - ayuda.getLocalBounds().width / 2.0f, 690.0f);
                    window.draw(ayuda);
            }

            window.display();
            continue;
        }

        // INPUT
        knight.handleInput();
        if (modoActual == MULTIJUGADOR)
        {
            knight2.handleInput(sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Right, sf::Keyboard::Left);
        }

        // PHYSICS STEP (Box2D v3.0)
        physics.step(timeStep);

        // SYNC GRAPHICS WITH PHYSICS
        knight.syncWithPhysics();
        if (modoActual == MULTIJUGADOR)
        {
            knight2.syncWithPhysics();
        }

        // UPDATE SPRITES
        knight.update();
        if (modoActual == MULTIJUGADOR)
        {
            knight2.update();
        }

        // UPDATE BOMBAS (FASE 2) + EXPLOSIONES (FASE 3)
        for (auto& bomba : listaBombas)
        {
            if (bomba.update(physics))  // Si retorna true = explotó
            {
                // Crear explosión con rango del Knight
                Explosion explosion(bomba.getFila(), bomba.getColumna(), mapa,
                                    bomba.getRangoExplosion(), &listaItems, bomba.getColorExplosion());
                listaExplosiones.push_back(explosion);
            }
        }

        // Eliminar bombas inactivas usando erase_if (C++20)
        // Alternativa manual si no está disponible:
        auto it = listaBombas.begin();
        while (it != listaBombas.end())
        {
            if (!it->isActiva())
            {
                it = listaBombas.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // UPDATE EXPLOSIONES (FASE 3)
        for (auto& explosion : listaExplosiones)
        {
            explosion.update();
        }

        // Eliminar explosiones inactivas
        auto it_exp = listaExplosiones.begin();
        while (it_exp != listaExplosiones.end())
        {
            if (!it_exp->isActiva())
            {
                it_exp = listaExplosiones.erase(it_exp);
            }
            else
            {
                ++it_exp;
            }
        }

        // UPDATE ENEMIGOS (FASE 4)
        if (modoActual == ARCADE)
        {
            for (auto& enemigo : listaEnemigos)
            {
                if (enemigo.isVivo())
                {
                    enemigo.update(mapa, physics);
                }
            }
        }

        // Eliminar enemigos muertos
        auto it_enem = listaEnemigos.begin();
        while (it_enem != listaEnemigos.end())
        {
            if (!it_enem->isVivo())
            {
                it_enem = listaEnemigos.erase(it_enem);
            }
            else
            {
                ++it_enem;
            }
        }

        // UPDATE BOSS (FASE 6)
        if (modoActual == ARCADE)
        {
            for (auto& boss : listaJefes)
            {
                boss.update(mapa, physics);
            }
        }

        // Daño a Boss por explosiones (FASE 6)
        if (modoActual == ARCADE)
        for (auto& boss : listaJefes)
        {
            b2Vec2 posBoss = b2Body_GetPosition(boss.getBodyId());
            int filaBoss = static_cast<int>((posBoss.y * PIXELS_PER_METER) / 64.0f);
            int colBoss = static_cast<int>((posBoss.x * PIXELS_PER_METER) / 64.0f);

            for (auto& explosion : listaExplosiones)
            {
                const auto& celdasExplosion = explosion.getCeldasAfectadas();
                for (const auto& celda : celdasExplosion)
                {
                    if (celda.x == filaBoss && celda.y == colBoss)
                    {
                        boss.recibirDano();
                        break;
                    }
                }
            }
        }

        // Eliminar jefes muertos (FASE 6)
        auto it_boss = listaJefes.begin();
        while (it_boss != listaJefes.end())
        {
            if (it_boss->getHp() <= 0)
            {
                it_boss->destruir(physics);
                it_boss = listaJefes.erase(it_boss);
            }
            else
            {
                ++it_boss;
            }
        }

        // Ataque de Boss al jugador (FASE 6)
        if (modoActual == ARCADE)
        for (auto& boss : listaJefes)
        {
            b2Vec2 posKnightBoss = b2Body_GetPosition(knight.getBodyId());
            float pixelXKnight = posKnightBoss.x * PIXELS_PER_METER;
            float pixelYKnight = posKnightBoss.y * PIXELS_PER_METER;

            b2Vec2 posBoss = b2Body_GetPosition(boss.getBodyId());
            float pixelXBoss = posBoss.x * PIXELS_PER_METER;
            float pixelYBoss = posBoss.y * PIXELS_PER_METER;

            // Distancia euclideana
            float distancia = std::sqrt((pixelXKnight - pixelXBoss) * (pixelXKnight - pixelXBoss) +
                                        (pixelYKnight - pixelYBoss) * (pixelYKnight - pixelYBoss));

            if (distancia < 64.0f)  // Contacto si distancia < 64px
            {
                knight.morir(physics);
            }
        }

        // LÓGICA DE VICTORIA/DERROTA (FASE 5 y 6)
        if (estadoActual == JUGANDO)
        {
            // Verificar si el jugador murió
            if (modoActual == MULTIJUGADOR && (knight.getVidas() <= 0 || knight2.getVidas() <= 0))
            {
                estadoActual = GAME_OVER;
                std::cout << "FIN DEL MULTIJUGADOR - Presiona ENTER para volver al menu" << std::endl;
            }
            else if (knight.getVidas() <= 0)
            {
                estadoActual = GAME_OVER;
                std::cout << "GAME OVER - Presiona ENTER para volver al menu" << std::endl;
            }
            // FASE 6: Si estamos en nivel 3, verificar si el Boss fue derrotado
            else if (modoActual == ARCADE && nivelActual == 3 && listaJefes.empty())
            {
                estadoActual = VICTORIA;
                std::cout << "¡VICTORIA! ¡Derrotaste al Jefe Final! - Presiona ENTER para reiniciar" << std::endl;
            }
            // Verificar si ganó el nivel (todos los enemigos muertos) - Niveles 1 y 2
            else if (modoActual == ARCADE && listaEnemigos.empty() && nivelActual < 3)
            {
                nivelActual++;
                std::cout << "¡NIVEL COMPLETADO! Avanzando a NIVEL " << nivelActual << std::endl;
                cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics);
            }
        }

        // No actualizar si no estamos jugando
        if (estadoActual != JUGANDO)
        {
            // Aun así renderizar, pero sin actualizar lógica
        }

        // DETECCIÓN DE COLISIONES LÓGICAS (POWER-UPS)
        b2Vec2 posKnight = b2Body_GetPosition(knight.getBodyId());
        float pixelXKnight = posKnight.x * PIXELS_PER_METER;
        float pixelYKnight = posKnight.y * PIXELS_PER_METER;
        int filaKnight = static_cast<int>(pixelYKnight / 64.0f);
        int colKnight = static_cast<int>(pixelXKnight / 64.0f);

        b2Vec2 posKnight2 = b2Body_GetPosition(knight2.getBodyId());
        float pixelXKnight2 = posKnight2.x * PIXELS_PER_METER;
        float pixelYKnight2 = posKnight2.y * PIXELS_PER_METER;
        int filaKnight2 = static_cast<int>(pixelYKnight2 / 64.0f);
        int colKnight2 = static_cast<int>(pixelXKnight2 / 64.0f);

        // Recopilar items
        for (auto& item : listaItems)
        {
            if (item.isActivo() && item.getFila() == filaKnight && item.getColumna() == colKnight)
            {
                knight.recolectarItem(item.getTipo());
                item.desactivar();
            }
        }

        // Eliminar items inactivos
        auto it_item = listaItems.begin();
        while (it_item != listaItems.end())
        {
            if (!it_item->isActivo())
            {
                it_item = listaItems.erase(it_item);
            }
            else
            {
                ++it_item;
            }
        }

        // Daño por fuego
        for (auto& explosion : listaExplosiones)
        {
            if (explosion.isActiva())
            {
                const auto& celdasAfectadas = explosion.getCeldasAfectadas();
                for (const auto& celda : celdasAfectadas)
                {
                    if (celda.x == filaKnight && celda.y == colKnight)
                    {
                        knight.morir(physics);
                        break;
                    }
                    if (modoActual == MULTIJUGADOR && celda.x == filaKnight2 && celda.y == colKnight2)
                    {
                        knight2.morir(physics);
                        break;
                    }
                }

                // Daño por fuego a enemigos (FASE 4)
                if (modoActual == ARCADE)
                for (auto& enemigo : listaEnemigos)
                {
                    if (enemigo.isVivo())
                    {
                        b2Vec2 posEnemigo = b2Body_GetPosition(enemigo.getBodyId());
                        float pixelXEnemigo = posEnemigo.x * PIXELS_PER_METER;
                        float pixelYEnemigo = posEnemigo.y * PIXELS_PER_METER;
                        int filaEnemigo = static_cast<int>(pixelYEnemigo / 64.0f);
                        int colEnemigo = static_cast<int>(pixelXEnemigo / 64.0f);

                        for (const auto& celda : celdasAfectadas)
                        {
                            if (celda.x == filaEnemigo && celda.y == colEnemigo)
                            {
                                enemigo.destruir(physics);
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Ataque de enemigos al jugador (FASE 4)
        if (modoActual == ARCADE)
        for (auto& enemigo : listaEnemigos)
        {
            if (enemigo.isVivo())
            {
                b2Vec2 posEnemigo = b2Body_GetPosition(enemigo.getBodyId());
                float pixelXEnemigo = posEnemigo.x * PIXELS_PER_METER;
                float pixelYEnemigo = posEnemigo.y * PIXELS_PER_METER;
                int filaEnemigo = static_cast<int>(pixelYEnemigo / 64.0f);
                int colEnemigo = static_cast<int>(pixelXEnemigo / 64.0f);

                if (filaEnemigo == filaKnight && colEnemigo == colKnight)
                {
                    knight.morir(physics);
                }
            }
        }

        // ACTUALIZAR HUD (FASE 5)
        if (fuenteCargada)
        {
            std::string hudText;
            if (modoActual == ARCADE)
            {
                hudText = "MODO: ARCADE | NIVEL: " + std::to_string(nivelActual) + 
                          " | VIDAS: " + std::to_string(knight.getVidas()) + 
                          " | ENEMIGOS: " + std::to_string(listaEnemigos.size());
            }
            else
            {
                hudText = "MODO: MULTIJUGADOR | P1 VIDAS: " + std::to_string(knight.getVidas()) +
                          " | P2 VIDAS: " + std::to_string(knight2.getVidas());
            }
            textoHUD.setString(hudText);
        }

        // RENDER
        window.clear(sf::Color(35, 35, 45));
        mapa.draw(window);
        
        // RENDER BOMBAS (FASE 2)
        for (auto& bomba : listaBombas)
        {
            bomba.draw(window);
        }

        // RENDER EXPLOSIONES (FASE 3)
        for (auto& explosion : listaExplosiones)
        {
            explosion.draw(window);
        }

        // RENDER POWER-UPS
        for (auto& item : listaItems)
        {
            item.draw(window);
        }

        // RENDER ENEMIGOS (FASE 4)
        for (auto& enemigo : listaEnemigos)
        {
            enemigo.draw(window);
        }

        // RENDER BOSS (FASE 6)
        for (auto& boss : listaJefes)
        {
            boss.draw(window);
        }
        
        knight.draw(window);
        if (modoActual == MULTIJUGADOR)
        {
            knight2.draw(window);
        }

        // RENDER HUD (FASE 5)
        if (fuenteCargada)
        {
            window.draw(textoHUD);
        }

        // RENDER GAME OVER / VICTORIA OVERLAY (FASE 5)
        if (estadoActual == GAME_OVER || estadoActual == VICTORIA)
        {
            // Rectángulo negro semi-transparente
            sf::RectangleShape overlay(sf::Vector2f(960.0f, 832.0f));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));  // Negro con transparencia
            window.draw(overlay);

            if (fuenteCargada)
            {
                if (estadoActual == GAME_OVER)
                {
                    window.draw(textoGameOver);
                }
                else if (estadoActual == VICTORIA)
                {
                    window.draw(textoVictoria);
                }
                window.draw(textoInstruccion);
            }
        }

        window.display();
    }

    return 0;
}
