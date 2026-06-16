#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <box2d/box2d.h>
#include <vector>
#include <map>
#include <string>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <utility>
#include "Mapa.hpp"
#include "Menu.hpp"
#include "SeleccionPersonaje.hpp"

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
    MENU_PRINCIPAL,
    SELECCION_PERSONAJE,
    JUGANDO,
    GAME_OVER,
    VICTORIA,
    VICTORIA_VERSUS
};

enum ModoJuego
{
    ARCADE,
    MULTIJUGADOR
};

// ============== PHYSICS ENGINE (Box2D v3.0) ==============
const float PIXELS_PER_METER = 30.0f;
const float TAMANO_TILE = 64.0f;
const float MAP_CELL_SIZE = TAMANO_TILE;
const float WALL_HITBOX_SIZE = 56.0f;
const float KNIGHT_HITBOX_SIZE = 40.0f;
const float KNIGHT_COLLISION_SIZE = TAMANO_TILE * 0.9f;
const float KNIGHT_COLLISION_RADIUS = KNIGHT_COLLISION_SIZE / 2.0f;
const float KNIGHT_DAMAGE_HITBOX_SIZE = 32.0f;
const float FIRE_DAMAGE_MARGIN = 4.0f;
const float ENEMY_HITBOX_SIZE = 36.0f;
const uint32_t COLLISION_DEFAULT = 0x0001;
const uint32_t COLLISION_PLAYER = 0x0002;
const uint32_t COLLISION_BOMB = 0x0004;
const uint32_t COLLISION_INDESTRUCTIBLE = 0x0008;
const uint32_t COLLISION_DESTRUCTIBLE = 0x0010;
const uint32_t COLLISION_ALL = 0xFFFFFFFF;
const int MAX_ITEMS_ACTIVOS = 3;
const int PROBABILIDAD_ITEM_DESTRUCTIBLE = 70;
const int MAX_CORAZONES_POR_MAPA = 2;
const float DURACION_ITEM_TEMPORAL = 10.0f;
const float DURACION_ESCUDO_VERSUS = 15.0f;
const float TIEMPO_RESPAWN_ITEM = 20.0f;
const float INCREMENTO_VELOCIDAD_VERSUS = 1.0f;
const float VELOCIDAD_BOMBA_PATEADA = 8.0f;

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

    b2BodyId createDynamicBody(float x, float y, float width, float height,
                               uint32_t categoryBits = COLLISION_DEFAULT,
                               uint32_t maskBits = COLLISION_ALL)
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
        shapeDef.friction = 0.3f;
        shapeDef.filter.categoryBits = categoryBits;
        shapeDef.filter.maskBits = maskBits;

        b2CreatePolygonShape(bodyId, &shapeDef, &polygon);

        return bodyId;
    }

    b2BodyId createDynamicCircleBody(float x, float y, float radius,
                                     uint32_t categoryBits = COLLISION_DEFAULT,
                                     uint32_t maskBits = COLLISION_ALL)
    {
        if (!b2World_IsValid(world))
        {
            return b2_nullBodyId;
        }

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.x = x / PIXELS_PER_METER;
        bodyDef.position.y = y / PIXELS_PER_METER;
        bodyDef.fixedRotation = true;

        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        b2Circle circle = {{0.0f, 0.0f}, radius / PIXELS_PER_METER};

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.friction = 0.0f;
        shapeDef.filter.categoryBits = categoryBits;
        shapeDef.filter.maskBits = maskBits;

        b2CreateCircleShape(bodyId, &shapeDef, &circle);

        return bodyId;
    }

    b2BodyId createStaticBody(float x, float y, float width, float height,
                              uint32_t categoryBits = COLLISION_DEFAULT,
                              uint32_t maskBits = COLLISION_ALL)
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
        shapeDef.filter.categoryBits = categoryBits;
        shapeDef.filter.maskBits = maskBits;

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
    Personaje(sf::Vector2f position, const std::string& rutaTextura)
    {
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

        cambiarTextura(rutaTextura);

        sprite.setPosition(position);

        // Cambia esto si se ve muy grande o muy pequeño
    }

    void cambiarTextura(const std::string& rutaTextura)
    {
        if (!textura.loadFromFile(rutaTextura))
        {
            std::cout << "Error: no se pudo cargar " << rutaTextura << std::endl;
            return;
        }

        sf::Vector2u size = textura.getSize();
        frameWidth = static_cast<int>(size.x) / numFrames;
        frameHeight = static_cast<int>(size.y) / 4;
        currentFrame = 0;
        direccion = ABAJO;
        caminando = false;
        caminandoAnterior = false;
        clock.restart();

        sprite.setTexture(this->textura);
        sprite.setTextureRect(sf::IntRect(
            currentFrame * frameWidth,
            obtenerFilaDireccion() * frameHeight,
            frameWidth,
            frameHeight - 2
        ));
        centrarSprite();
        float escala = MAP_CELL_SIZE / static_cast<float>(frameHeight);
        sprite.setScale(escala, escala);
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
            frameHeight - 2
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
            frameHeight - 2
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
        return 3; // derecha
    }
    else
    {
        return 2; // izquierda
    }
}

    void centrarSprite()
    {
        sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
    }
};

// ============== POWER-UP ==============
enum TipoPowerUp
{
    ITEM_BOMBA_EXTRA = 0,
    ITEM_ESCUDO = 1,
    ITEM_FANTASMA = 2,
    ITEM_VELOCIDAD = 3,
    ITEM_VIDA = 4,
    ITEM_PATEAR = 5,
    ITEM_TOTAL = 6
};

class PowerUp
{
public:
    PowerUp(int fila, int columna, int tipo = -1, bool visible = false)
        : fila(fila), columna(columna), tipo(tipo), activo(true), visible(visible)
    {
        if (this->tipo < 0 || this->tipo >= ITEM_TOTAL)
        {
            this->tipo = rand() % ITEM_TOTAL;
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (!activo || !visible)
        {
            return;
        }

        sf::Texture* textura = obtenerTextura(tipo);
        float posX = columna * 64.0f + 32.0f;
        float posY = fila * 64.0f + 32.0f;

        if (textura != nullptr)
        {
            sf::Sprite item;
            item.setTexture(*textura);
            sf::Vector2u size = textura->getSize();
            item.setOrigin(size.x / 2.0f, size.y / 2.0f);
            float escala = 46.0f / static_cast<float>(std::max(size.x, size.y));
            item.setScale(escala, escala);
            item.setPosition(posX, posY);
            window.draw(item);
            return;
        }

        if (activo)
        {
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
    bool estaVisible() const { return visible; }
    bool estaOculto() const { return activo && !visible; }
    bool estaEnCelda(int f, int c) const { return fila == f && columna == c; }
    void revelar() { visible = true; }
    void desactivar() { activo = false; }

    static sf::Texture* obtenerTextura(int tipo)
    {
        cargarTexturas();
        if (tipo < 0 || tipo >= ITEM_TOTAL || !texturasCargadas[tipo])
        {
            return nullptr;
        }

        return &texturas[tipo];
    }

    static const char* obtenerNombre(int tipo)
    {
        static const char* nombres[ITEM_TOTAL] = {
            "Bomba",
            "Escudo",
            "Fantasma",
            "Velocidad",
            "Vida",
            "Patear"
        };

        if (tipo < 0 || tipo >= ITEM_TOTAL)
        {
            return "";
        }

        return nombres[tipo];
    }

private:
    static void cargarTexturas()
    {
        static const char* rutas[ITEM_TOTAL] = {
            "assets/images/item_bomba.png",
            "assets/images/item_escudo.png",
            "assets/images/item_fantasma.png",
            "assets/images/item_velocidad.png",
            "assets/images/item_vida.png",
            "assets/images/item_patear.png"
        };

        for (int i = 0; i < ITEM_TOTAL; i++)
        {
            if (!texturasIntentadas[i])
            {
                texturasCargadas[i] = texturas[i].loadFromFile(rutas[i]);
                texturasIntentadas[i] = true;
                if (!texturasCargadas[i])
                {
                    std::cout << "Aviso: no se pudo cargar " << rutas[i] << std::endl;
                }
            }
        }
    }

    int fila;
    int columna;
    int tipo;  // TipoPowerUp
    bool activo;
    bool visible;

    static std::array<sf::Texture, ITEM_TOTAL> texturas;
    static std::array<bool, ITEM_TOTAL> texturasCargadas;
    static std::array<bool, ITEM_TOTAL> texturasIntentadas;
};

std::array<sf::Texture, ITEM_TOTAL> PowerUp::texturas;
std::array<bool, ITEM_TOTAL> PowerUp::texturasCargadas = {false, false, false, false, false, false};
std::array<bool, ITEM_TOTAL> PowerUp::texturasIntentadas = {false, false, false, false, false, false};

// ============== FIN POWER-UP ==============

#include "Mapa.cpp"
#include "Menu.cpp"
#include "SeleccionPersonaje.cpp"

// ============== EXPLOSIÓN (FASE 3: Explosiones y Destrucción) ==============
class Explosion
{
public:
    Explosion(int centroFila, int centroCol, Mapa& mapa, int rango = 2,
              std::vector<PowerUp>* pItems = nullptr,
              sf::Color colorBase = sf::Color(255, 165, 0, 200))
        : colorBase(colorBase), activa(true), pListaItems(pItems)
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
            if (tipo == Mapa::INDESTRUCTIBLE || tipo == Mapa::DESTRUYENDOSE)
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
            if (tipo == Mapa::INDESTRUCTIBLE || tipo == Mapa::DESTRUYENDOSE)
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
            if (tipo == Mapa::INDESTRUCTIBLE || tipo == Mapa::DESTRUYENDOSE)
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
            if (tipo == Mapa::INDESTRUCTIBLE || tipo == Mapa::DESTRUYENDOSE)
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
        if (activa && temporizador.getElapsedTime().asSeconds() >= DURACION_EXPLOSION)
        {
            activa = false;
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (activa)
        {
            float progreso = temporizador.getElapsedTime().asSeconds() / DURACION_EXPLOSION;
            if (progreso > 1.0f)
            {
                progreso = 1.0f;
            }

            sf::Color colorActual = colorBase;
            colorActual.a = static_cast<sf::Uint8>(std::max(0.0f, static_cast<float>(colorBase.a) * (1.0f - progreso)));

            for (const auto& celda : celdasAfectadas)
            {
                sf::RectangleShape fuego(sf::Vector2f(64.0f, 64.0f));
                fuego.setFillColor(colorActual);
                fuego.setPosition(celda.y * 64.0f, celda.x * 64.0f);
                window.draw(fuego);
            }
        }
    }

    bool isActiva() const { return activa; }

    const std::vector<sf::Vector2i>& getCeldasAfectadas() const { return celdasAfectadas; }

private:
    static constexpr float DURACION_EXPLOSION = Mapa::DURACION_DESTRUCCION_BLOQUE;
    std::vector<sf::Vector2i> celdasAfectadas;
    sf::Clock temporizador;
    sf::Color colorBase;
    bool activa;
    std::vector<PowerUp>* pListaItems;
};

// ============== FIN EXPLOSIÓN ==============

// ============== BOMBA (FASE 2: Sistema de Bombas) ==============
class Bomba
{
public:
    Bomba(int fila, int columna, PhysicsSpace& physics, int rangoFuego = 1, int propietario = 0,
          const std::string& rutaTextura = "assets/images/Bomba_Verde.png")
        : fila(fila), columna(columna), activa(true), jugadorEncima(true), solida(false),
          enMovimiento(false), direccionMovimiento(ABAJO), rangoFuego(rangoFuego), propietario(propietario),
          bodyId(b2_nullBodyId), shapeId(b2_nullShapeId), texturaCargada(false)
    {
        cargarTextura(rutaTextura);
        // Calcular posición en píxeles: centro de la celda
        float posX = columna * 64.0f + 32.0f;
        float posY = fila * 64.0f + 32.0f;

        // Crear cuerpo estático en Box2D v3.0 (48x48 para la bomba)
        b2WorldId worldId = physics.getWorldId();
        if (b2World_IsValid(worldId))
        {
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_staticBody;
            bodyDef.position.x = posX / PIXELS_PER_METER;
            bodyDef.position.y = posY / PIXELS_PER_METER;
            bodyId = b2CreateBody(worldId, &bodyDef);

            b2Polygon polygon = b2MakeBox(
                24.0f / PIXELS_PER_METER,
                24.0f / PIXELS_PER_METER
            );

            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 1.0f;
            shapeDef.filter.categoryBits = COLLISION_BOMB;
            shapeDef.filter.maskBits = COLLISION_ALL & ~COLLISION_PLAYER;
            shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
        }

        // Iniciar temporizador
        temporizador.restart();
    }

    Bomba(const Bomba&) = delete;
    Bomba& operator=(const Bomba&) = delete;

    Bomba(Bomba&& otra) noexcept
        : fila(otra.fila), columna(otra.columna), activa(otra.activa),
          jugadorEncima(otra.jugadorEncima), solida(otra.solida),
          enMovimiento(otra.enMovimiento), direccionMovimiento(otra.direccionMovimiento),
          rangoFuego(otra.rangoFuego), propietario(otra.propietario),
          bodyId(otra.bodyId), shapeId(otra.shapeId),
          textura(std::move(otra.textura)), sprite(std::move(otra.sprite)),
          texturaCargada(otra.texturaCargada), temporizador(std::move(otra.temporizador))
    {
        enlazarSpriteATextura();
        otra.bodyId = b2_nullBodyId;
        otra.shapeId = b2_nullShapeId;
        otra.activa = false;
    }

    Bomba& operator=(Bomba&& otra) noexcept
    {
        if (this != &otra)
        {
            destruirFisica();

            fila = otra.fila;
            columna = otra.columna;
            activa = otra.activa;
            jugadorEncima = otra.jugadorEncima;
            solida = otra.solida;
            enMovimiento = otra.enMovimiento;
            direccionMovimiento = otra.direccionMovimiento;
            rangoFuego = otra.rangoFuego;
            propietario = otra.propietario;
            bodyId = otra.bodyId;
            shapeId = otra.shapeId;
            textura = std::move(otra.textura);
            sprite = std::move(otra.sprite);
            texturaCargada = otra.texturaCargada;
            temporizador = std::move(otra.temporizador);
            enlazarSpriteATextura();

            otra.bodyId = b2_nullBodyId;
            otra.shapeId = b2_nullShapeId;
            otra.activa = false;
        }

        return *this;
    }

    bool update(PhysicsSpace& physics, const std::vector<b2BodyId>& jugadoresBodyId)
    {
        // Verificar si superó los 3 segundos
        (void)physics;

        if (activa && jugadorEncima && !solida)
        {
            float bombaX = columna * 64.0f + 32.0f;
            float bombaY = fila * 64.0f + 32.0f;
            bool algunJugadorEncima = false;

            for (b2BodyId jugadorBodyId : jugadoresBodyId)
            {
                if (!b2Body_IsValid(jugadorBodyId))
                {
                    continue;
                }

                b2Vec2 posJugador = b2Body_GetPosition(jugadorBodyId);
                float jugadorX = posJugador.x * PIXELS_PER_METER;
                float jugadorY = posJugador.y * PIXELS_PER_METER;

                if (std::abs(jugadorX - bombaX) < 48.0f &&
                    std::abs(jugadorY - bombaY) < 48.0f)
                {
                    algunJugadorEncima = true;
                    break;
                }
            }

            if (!algunJugadorEncima)
            {
                jugadorEncima = false;
                solida = true;

                if (b2Shape_IsValid(shapeId))
                {
                    b2Filter filtroSolido = b2Shape_GetFilter(shapeId);
                    filtroSolido.maskBits = COLLISION_ALL;
                    b2Shape_SetFilter(shapeId, filtroSolido);
                }
            }
        }

        if (enMovimiento)
        {
            actualizarCeldaDesdeFisica();
        }

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

    bool update(PhysicsSpace& physics, b2BodyId jugadorBodyId)
    {
        std::vector<b2BodyId> jugadoresBodyId = {jugadorBodyId};
        return update(physics, jugadoresBodyId);
    }

    void draw(sf::RenderWindow& window)
    {
        if (activa)
        {
            // Calcular posición en píxeles: centro de la celda
            float posX = columna * 64.0f + 32.0f;
            float posY = fila * 64.0f + 32.0f;
            if (b2Body_IsValid(bodyId))
            {
                b2Vec2 pos = b2Body_GetPosition(bodyId);
                posX = pos.x * PIXELS_PER_METER;
                posY = pos.y * PIXELS_PER_METER;
            }

            // Dibujar círculo negro (radio 24px)
            if (texturaCargada)
            {
                sprite.setPosition(posX, posY);
                window.draw(sprite);
                return;
            }

            sf::CircleShape bomba(24.0f);
            bomba.setFillColor(sf::Color::Black);
            bomba.setPosition(posX - 24.0f, posY - 24.0f);  // Centrar
            window.draw(bomba);
        }
    }

    void actualizarCeldaDesdeFisica()
    {
        if (!b2Body_IsValid(bodyId))
        {
            return;
        }

        b2Vec2 pos = b2Body_GetPosition(bodyId);
        float pixelX = pos.x * PIXELS_PER_METER;
        float pixelY = pos.y * PIXELS_PER_METER;
        columna = static_cast<int>(pixelX / 64.0f);
        fila = static_cast<int>(pixelY / 64.0f);
    }

    void patear(Direccion direccion)
    {
        if (!activa || !solida || enMovimiento || !b2Body_IsValid(bodyId))
        {
            return;
        }

        direccionMovimiento = direccion;
        enMovimiento = true;
        b2Body_SetType(bodyId, b2_dynamicBody);
        b2Body_SetLinearVelocity(bodyId, obtenerVelocidadDireccion(direccionMovimiento));
    }

    void detenerMovimiento()
    {
        if (!b2Body_IsValid(bodyId))
        {
            return;
        }

        actualizarCeldaDesdeFisica();
        float posX = columna * 64.0f + 32.0f;
        float posY = fila * 64.0f + 32.0f;
        b2Transform transform = {{posX / PIXELS_PER_METER, posY / PIXELS_PER_METER}, b2Rot_identity};
        b2Body_SetTransform(bodyId, transform.p, transform.q);
        b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
        b2Body_SetType(bodyId, b2_staticBody);
        enMovimiento = false;
    }

    void actualizarMovimientoPateado(const Mapa& mapa)
    {
        if (!activa || !enMovimiento || !b2Body_IsValid(bodyId))
        {
            return;
        }

        actualizarCeldaDesdeFisica();
        int siguienteFila = fila;
        int siguienteColumna = columna;

        if (direccionMovimiento == ARRIBA)
            siguienteFila--;
        else if (direccionMovimiento == ABAJO)
            siguienteFila++;
        else if (direccionMovimiento == DERECHA)
            siguienteColumna++;
        else if (direccionMovimiento == IZQUIERDA)
            siguienteColumna--;

        if (mapa.obtenerTipoCelda(siguienteFila, siguienteColumna) != Mapa::VACIO)
        {
            detenerMovimiento();
            return;
        }

        b2Vec2 velocidad = b2Body_GetLinearVelocity(bodyId);
        float rapidez = std::sqrt(velocidad.x * velocidad.x + velocidad.y * velocidad.y);
        if (rapidez < 0.25f)
        {
            detenerMovimiento();
        }
    }

    bool isActiva() const { return activa; }
    bool estaSolida() const { return solida; }
    bool estaEnMovimiento() const { return enMovimiento; }

    void destruirFisica()
    {
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }

        bodyId = b2_nullBodyId;
        shapeId = b2_nullShapeId;
    }

    int getFila() const { return fila; }
    int getColumna() const { return columna; }
    int getRangoFuego() const { return rangoFuego; }
    int getPropietario() const { return propietario; }
    b2BodyId getBodyId() const { return bodyId; }

private:
    void cargarTextura(const std::string& rutaTextura)
    {
        texturaCargada = textura.loadFromFile(rutaTextura);
        if (!texturaCargada)
        {
            std::cout << "Aviso: no se pudo cargar " << rutaTextura << std::endl;
            return;
        }

        enlazarSpriteATextura();
    }

    void enlazarSpriteATextura()
    {
        if (!texturaCargada)
        {
            return;
        }

        sprite.setTexture(textura, true);
        sf::Vector2u size = textura.getSize();
        sprite.setOrigin(size.x / 2.0f, size.y / 2.0f);
        float escala = MAP_CELL_SIZE / static_cast<float>(std::max(size.x, size.y));
        sprite.setScale(escala, escala);
    }

    b2Vec2 obtenerVelocidadDireccion(Direccion direccion) const
    {
        if (direccion == ARRIBA)
            return {0.0f, -VELOCIDAD_BOMBA_PATEADA};
        if (direccion == ABAJO)
            return {0.0f, VELOCIDAD_BOMBA_PATEADA};
        if (direccion == DERECHA)
            return {VELOCIDAD_BOMBA_PATEADA, 0.0f};
        return {-VELOCIDAD_BOMBA_PATEADA, 0.0f};
    }

    int fila;
    int columna;
    bool activa;
    bool jugadorEncima;
    bool solida;
    bool enMovimiento;
    Direccion direccionMovimiento;
    int rangoFuego;
    int propietario;
    b2BodyId bodyId;
    b2ShapeId shapeId;
    sf::Texture textura;
    sf::Sprite sprite;
    bool texturaCargada;
    sf::Clock temporizador;
};

// ============== FIN BOMBA ==============

// ============== KNIGHT (JUGADOR) - Box2D v3.0 ==============
class Knight
{
public:
    Knight(sf::Vector2f position, PhysicsSpace& physics,
           const std::string& rutaTextura = "assets/images/Verde_spritesheet.png",
           int idJugador = 1)
        : personaje(position, rutaTextura), physicsSpace(physics), 
          maxBombas(1), rangoFuego(1), speed(6.5f),
          speedOriginal(6.5f), vidas(3), tiempoInvulnerable(0.0f),
          tiempoBombaExtra(0.0f), tiempoEscudo(0.0f), tiempoFantasma(0.0f),
          tiempoVelocidad(0.0f), bombasExtraRonda(0), velocidadExtraRonda(0.0f),
          fantasmaActivo(false), puedePatear(false), ignorandoDestructibles(false),
          idJugador(idJugador), rutaTexturaBomba("assets/images/Bomba_Verde.png")
    {
        // Crear cuerpo dinámico en Box2D v3.0
        bodyId = physicsSpace.createDynamicCircleBody(
            position.x, 
            position.y, 
            KNIGHT_COLLISION_RADIUS,
            COLLISION_PLAYER,
            COLLISION_ALL & ~COLLISION_PLAYER
        );
        
        posicionOriginal = position;
    }

    void handleInput()
    {
        if (idJugador == 2)
        {
            handleInput(sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Right, sf::Keyboard::Left);
        }
        else
        {
            handleInput(sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::D, sf::Keyboard::A);
        }
    }

    void handleInput(sf::Keyboard::Key teclaArriba,
                     sf::Keyboard::Key teclaAbajo,
                     sf::Keyboard::Key teclaDerecha,
                     sf::Keyboard::Key teclaIzquierda)
    {
        // Usar la velocidad del Knight (modificable por power-ups)
        
        b2Vec2 velocity = {0.0f, 0.0f};
        Direccion dirActual = direccionActual;
        bool seMovio = false;

        if (sf::Keyboard::isKeyPressed(teclaArriba))
        {
            velocity.y = -speed;
            dirActual = ARRIBA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(teclaAbajo))
        {
            velocity.y = speed;
            dirActual = ABAJO;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(teclaDerecha))
        {
            velocity.x = speed;
            dirActual = DERECHA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(teclaIzquierda))
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

    void update(float deltaTime)
    {
        if (tiempoInvulnerable > 0.0f)
        {
            tiempoInvulnerable -= deltaTime;
            if (tiempoInvulnerable < 0.0f)
            {
                tiempoInvulnerable = 0.0f;
            }
        }

        actualizarTemporizador(tiempoEscudo, deltaTime);

        if (bombasExtraRonda <= 0)
        {
            actualizarTemporizador(tiempoBombaExtra, deltaTime);
        }
        if (velocidadExtraRonda <= 0.0f)
        {
            actualizarTemporizador(tiempoVelocidad, deltaTime);
        }
        if (!fantasmaActivo)
        {
            actualizarTemporizador(tiempoFantasma, deltaTime);
        }

        maxBombas = 1 + bombasExtraRonda;
        if (tiempoBombaExtra > 0.0f)
        {
            maxBombas = std::max(maxBombas, 2);
        }

        speed = speedOriginal + velocidadExtraRonda;
        if (tiempoVelocidad > 0.0f)
        {
            speed = std::max(speed, speedOriginal * 1.5f);
        }

        setIgnorarDestructibles(fantasmaActivo || tiempoFantasma > 0.0f);

        personaje.update();
    }

    void draw(sf::RenderWindow& window)
    {
        personaje.draw(window);
    }

    void cambiarSprite(const std::string& rutaTextura)
    {
        personaje.cambiarTextura(rutaTextura);
        direccionActual = ABAJO;
        personaje.detenerAnimacion(direccionActual);
    }

    void configurarBomba(const std::string& rutaTextura)
    {
        rutaTexturaBomba = rutaTextura;
    }

    void plantarBomba(Mapa& mapa, std::vector<Bomba>& bombas, PhysicsSpace& physics, int propietario = 0)
    {
        // Verificar que no se supere el máximo de bombas
        int bombasActivas = 0;
        for (const auto& bomba : bombas)
        {
            if (bomba.isActiva() && bomba.getPropietario() == propietario)
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

        bombas.emplace_back(fila, columna, physics, rangoFuego, propietario, rutaTexturaBomba);
    }

    void recolectarItem(int tipo, bool modoVersus = false)
    {
        if (tipo == ITEM_BOMBA_EXTRA)
        {
            // Bota: aumentar velocidad
            if (modoVersus)
            {
                bombasExtraRonda++;
                maxBombas = 1 + bombasExtraRonda;
            }
            else
            {
                tiempoBombaExtra = DURACION_ITEM_TEMPORAL;
                maxBombas = 2;
            }
        }
        else if (tipo == ITEM_ESCUDO)
        {
            // Fuego: aumentar rango
            tiempoEscudo = modoVersus ? DURACION_ESCUDO_VERSUS : DURACION_ITEM_TEMPORAL;
        }
        else if (tipo == ITEM_FANTASMA)
        {
            // Bomba Extra: aumentar máximo de bombas
            if (modoVersus)
            {
                fantasmaActivo = true;
                tiempoFantasma = 0.0f;
            }
            else
            {
                tiempoFantasma = DURACION_ITEM_TEMPORAL;
            }
            setIgnorarDestructibles(true);
        }
        else if (tipo == ITEM_VELOCIDAD)
        {
            if (modoVersus)
            {
                velocidadExtraRonda += INCREMENTO_VELOCIDAD_VERSUS;
                speed = speedOriginal + velocidadExtraRonda;
            }
            else
            {
                tiempoVelocidad = DURACION_ITEM_TEMPORAL;
                speed = speedOriginal * 1.5f;
            }
        }
        else if (tipo == ITEM_VIDA)
        {
            vidas++;
        }
        else if (tipo == ITEM_PATEAR)
        {
            puedePatear = true;
        }
    }

    bool morir(PhysicsSpace& physics, bool conservarMejoras = false)
    {
        return recibirDano(physics, conservarMejoras);
    }

    bool recibirDano(PhysicsSpace& physics, bool conservarMejoras = false)
    {
        (void)physics;
        if (vidas <= 0 || tiempoInvulnerable > 0.0f || tiempoEscudo > 0.0f)
        {
            return false;
        }

        vidas--;
        tiempoInvulnerable = 2.0f;

        if (vidas > 0)
        {
            float posX = posicionOriginal.x;
            float posY = posicionOriginal.y;

            if (b2Body_IsValid(bodyId))
            {
                b2Transform transform = {{posX / PIXELS_PER_METER, posY / PIXELS_PER_METER}, b2Rot_identity};
                b2Body_SetTransform(bodyId, transform.p, transform.q);
                b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
            }

            if (!conservarMejoras)
            {
                maxBombas = 1;
                rangoFuego = 1;
                speed = speedOriginal;
                limpiarEfectosTemporales();
            }
        }

        return true;
    }

    void reiniciar(float x, float y)
    {
        posicionOriginal = sf::Vector2f(x, y);

        if (b2Body_IsValid(bodyId))
        {
            b2Transform transform = {{x / PIXELS_PER_METER, y / PIXELS_PER_METER}, b2Rot_identity};
            b2Body_SetTransform(bodyId, transform.p, transform.q);
            b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
        }

        personaje.setPhysicsPosition(x, y);
        maxBombas = 1;
        rangoFuego = 1;
        speed = speedOriginal;
        vidas = 3;
        tiempoInvulnerable = 0.0f;
        limpiarEfectosTemporales();
    }

    void reiniciarRonda(float x, float y)
    {
        posicionOriginal = sf::Vector2f(x, y);

        if (b2Body_IsValid(bodyId))
        {
            b2Transform transform = {{x / PIXELS_PER_METER, y / PIXELS_PER_METER}, b2Rot_identity};
            b2Body_SetTransform(bodyId, transform.p, transform.q);
            b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
        }

        personaje.setPhysicsPosition(x, y);
        direccionActual = ABAJO;
        personaje.detenerAnimacion(direccionActual);
        tiempoInvulnerable = 0.0f;
        limpiarInventarioRonda();
    }

    b2BodyId getBodyId() { return bodyId; }
    int getRangoFuego() const { return rangoFuego; }
    int getMaxBombas() const { return maxBombas; }
    int getVidas() const { return vidas; }
    void setVidas(int nuevasVidas) { vidas = nuevasVidas; }
    Direccion getDireccionActual() const { return direccionActual; }
    bool puedePatearBombas() const { return puedePatear; }
    bool tieneFantasma() const { return fantasmaActivo || tiempoFantasma > 0.0f; }
    float getTiempoItem(int tipo) const
    {
        if (tipo == ITEM_BOMBA_EXTRA)
            return bombasExtraRonda > 0 ? 1.0f : tiempoBombaExtra;
        if (tipo == ITEM_ESCUDO)
            return tiempoEscudo;
        if (tipo == ITEM_FANTASMA)
            return fantasmaActivo ? 1.0f : tiempoFantasma;
        if (tipo == ITEM_VELOCIDAD)
            return velocidadExtraRonda > 0.0f ? 1.0f : tiempoVelocidad;
        if (tipo == ITEM_PATEAR)
            return puedePatear ? 1.0f : 0.0f;
        return 0.0f;
    }

private:
    void actualizarTemporizador(float& tiempo, float deltaTime)
    {
        if (tiempo > 0.0f)
        {
            tiempo -= deltaTime;
            if (tiempo < 0.0f)
            {
                tiempo = 0.0f;
            }
        }
    }

    void limpiarEfectosTemporales()
    {
        tiempoBombaExtra = 0.0f;
        tiempoEscudo = 0.0f;
        tiempoFantasma = 0.0f;
        tiempoVelocidad = 0.0f;
        bombasExtraRonda = 0;
        velocidadExtraRonda = 0.0f;
        fantasmaActivo = false;
        puedePatear = false;
        maxBombas = 1;
        speed = speedOriginal;
        setIgnorarDestructibles(false);
    }

    void limpiarInventarioRonda()
    {
        limpiarEfectosTemporales();
        rangoFuego = 1;
    }

    void setIgnorarDestructibles(bool activo)
    {
        if (ignorandoDestructibles == activo || !b2Body_IsValid(bodyId))
        {
            return;
        }

        int cantidadShapes = b2Body_GetShapeCount(bodyId);
        if (cantidadShapes <= 0)
        {
            return;
        }

        std::vector<b2ShapeId> shapes(cantidadShapes);
        int shapesLeidas = b2Body_GetShapes(bodyId, shapes.data(), cantidadShapes);
        uint32_t maskBits = activo
            ? (COLLISION_ALL & ~COLLISION_DESTRUCTIBLE & ~COLLISION_BOMB & ~COLLISION_PLAYER)
            : (COLLISION_ALL & ~COLLISION_PLAYER);

        for (int i = 0; i < shapesLeidas; i++)
        {
            if (b2Shape_IsValid(shapes[i]))
            {
                b2Filter filtro = b2Shape_GetFilter(shapes[i]);
                filtro.maskBits = maskBits;
                b2Shape_SetFilter(shapes[i], filtro);
            }
        }

        ignorandoDestructibles = activo;
    }

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
    float tiempoInvulnerable;
    float tiempoBombaExtra;
    float tiempoEscudo;
    float tiempoFantasma;
    float tiempoVelocidad;
    int bombasExtraRonda;
    float velocidadExtraRonda;
    bool fantasmaActivo;
    bool puedePatear;
    bool ignorandoDestructibles;
    int idJugador;
    std::string rutaTexturaBomba;
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
        bodyId = physics.createDynamicBody(x, y, 48.0f, 48.0f);

        // Cargar textura y sprite
        if (!textura.loadFromFile(texturePath))
        {
            std::cerr << "Error: No se pudo cargar textura: " << texturePath << std::endl;
        }

        sprite.setTexture(this->textura);
        sprite.setScale(0.25f, 0.25f);
        sprite.setPosition(x - 12.0f, y - 12.0f);  // Centrar sprite
    }

    Enemigo(const Enemigo& otro)
        : bodyId(otro.bodyId),
          sprite(otro.sprite),
          textura(otro.textura),
          dirActual(otro.dirActual),
          speed(otro.speed),
          vivo(otro.vivo)
    {
        sprite.setTexture(this->textura);
    }

    Enemigo& operator=(const Enemigo& otro)
    {
        if (this != &otro)
        {
            bodyId = otro.bodyId;
            sprite = otro.sprite;
            textura = otro.textura;
            dirActual = otro.dirActual;
            speed = otro.speed;
            vivo = otro.vivo;
            sprite.setTexture(this->textura);
        }

        return *this;
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
        (void)physics;
        vivo = false;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
        bodyId = b2_nullBodyId;
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

        sprite.setTexture(this->textura);
        sprite.setScale(0.5f, 0.5f);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, 
                         sprite.getLocalBounds().height / 2.0f);
    }

    Boss(const Boss& otro)
        : bodyId(otro.bodyId),
          sprite(otro.sprite),
          textura(otro.textura),
          hp(otro.hp),
          cooldownDano(otro.cooldownDano),
          dirActual(otro.dirActual),
          speed(otro.speed)
    {
        sprite.setTexture(this->textura);
    }

    Boss& operator=(const Boss& otro)
    {
        if (this != &otro)
        {
            bodyId = otro.bodyId;
            sprite = otro.sprite;
            textura = otro.textura;
            hp = otro.hp;
            cooldownDano = otro.cooldownDano;
            dirActual = otro.dirActual;
            speed = otro.speed;
            sprite.setTexture(this->textura);
        }

        return *this;
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
        (void)physics;
        if (b2Body_IsValid(bodyId))
        {
            b2DestroyBody(bodyId);
        }
        bodyId = b2_nullBodyId;
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
int contarItemsActivos(const std::vector<PowerUp>& items)
{
    int total = 0;
    for (const auto& item : items)
    {
        if (item.isActivo())
        {
            total++;
        }
    }
    return total;
}

bool celdaTieneItemActivo(const std::vector<PowerUp>& items, int fila, int columna)
{
    for (const auto& item : items)
    {
        if (item.isActivo() && item.estaEnCelda(fila, columna))
        {
            return true;
        }
    }
    return false;
}

int seleccionarTipoItemBalanceado(int& corazonesSpawneados)
{
    int tipo = rand() % ITEM_TOTAL;
    if (tipo == ITEM_VIDA)
    {
        if (corazonesSpawneados < MAX_CORAZONES_POR_MAPA)
        {
            corazonesSpawneados++;
            return ITEM_VIDA;
        }

        return (rand() % 2 == 0) ? ITEM_BOMBA_EXTRA : ITEM_VELOCIDAD;
    }

    return tipo;
}

bool colocarItemOcultoAleatorio(Mapa& mapa, std::vector<PowerUp>& items, int& corazonesSpawneados)
{
    std::vector<sf::Vector2i> celdasDisponibles;
    for (int fila = 1; fila < 12; fila++)
    {
        for (int columna = 1; columna < 14; columna++)
        {
            if (mapa.obtenerTipoCelda(fila, columna) == Mapa::DESTRUCTIBLE &&
                !celdaTieneItemActivo(items, fila, columna))
            {
                celdasDisponibles.push_back({fila, columna});
            }
        }
    }

    if (celdasDisponibles.empty())
    {
        return false;
    }

    sf::Vector2i celda = celdasDisponibles[rand() % celdasDisponibles.size()];
    items.emplace_back(celda.x, celda.y, seleccionarTipoItemBalanceado(corazonesSpawneados), false);
    return true;
}

void llenarItemsIniciales(Mapa& mapa, std::vector<PowerUp>& items, int& corazonesSpawneados)
{
    corazonesSpawneados = 0;

    for (int fila = 1; fila < 12; fila++)
    {
        for (int columna = 1; columna < 14; columna++)
        {
            if (mapa.obtenerTipoCelda(fila, columna) == Mapa::DESTRUCTIBLE &&
                !celdaTieneItemActivo(items, fila, columna) &&
                (rand() % 100) < PROBABILIDAD_ITEM_DESTRUCTIBLE)
            {
                items.emplace_back(fila, columna, seleccionarTipoItemBalanceado(corazonesSpawneados), false);
            }
        }
    }
}

void dibujarItemsActivosHUD(sf::RenderWindow& window, const Knight& knight, sf::Font& fuente,
                            const std::string& etiqueta, float x, float y)
{
    const int tiposTemporales[5] = {
        ITEM_BOMBA_EXTRA,
        ITEM_ESCUDO,
        ITEM_FANTASMA,
        ITEM_VELOCIDAD,
        ITEM_PATEAR
    };

    float offsetX = 0.0f;
    bool dibujoAlgo = false;

    for (int tipo : tiposTemporales)
    {
        float tiempo = knight.getTiempoItem(tipo);
        if (tiempo <= 0.0f)
        {
            continue;
        }

        if (!dibujoAlgo)
        {
            sf::Text textoEtiqueta;
            textoEtiqueta.setFont(fuente);
            textoEtiqueta.setCharacterSize(16);
            textoEtiqueta.setFillColor(sf::Color::White);
            textoEtiqueta.setOutlineColor(sf::Color::Black);
            textoEtiqueta.setOutlineThickness(2.0f);
            textoEtiqueta.setString(etiqueta);
            textoEtiqueta.setPosition(x, y + 8.0f);
            window.draw(textoEtiqueta);
            offsetX = 34.0f;
            dibujoAlgo = true;
        }

        sf::RectangleShape fondoIcono(sf::Vector2f(72.0f, 36.0f));
        fondoIcono.setFillColor(sf::Color(0, 0, 0, 135));
        fondoIcono.setOutlineColor(sf::Color(255, 255, 255, 90));
        fondoIcono.setOutlineThickness(1.0f);
        fondoIcono.setPosition(x + offsetX, y);
        window.draw(fondoIcono);

        sf::Texture* textura = PowerUp::obtenerTextura(tipo);
        if (textura != nullptr)
        {
            sf::Sprite icono;
            icono.setTexture(*textura);
            sf::Vector2u size = textura->getSize();
            icono.setOrigin(size.x / 2.0f, size.y / 2.0f);
            float escala = 28.0f / static_cast<float>(std::max(size.x, size.y));
            icono.setScale(escala, escala);
            icono.setPosition(x + offsetX + 18.0f, y + 18.0f);
            window.draw(icono);
        }

        sf::Text textoTiempo;
        textoTiempo.setFont(fuente);
        textoTiempo.setCharacterSize(16);
        textoTiempo.setFillColor(sf::Color::White);
        textoTiempo.setOutlineColor(sf::Color::Black);
        textoTiempo.setOutlineThickness(2.0f);
        textoTiempo.setString(std::to_string(static_cast<int>(std::ceil(tiempo))) + "s");
        textoTiempo.setPosition(x + offsetX + 38.0f, y + 8.0f);
        window.draw(textoTiempo);

        offsetX += 78.0f;
    }
}

void cargarNivel(int nivel, Mapa& mapa, Knight& knight, std::vector<Enemigo>& enemigos,
                 std::vector<Bomba>& bombas, std::vector<Explosion>& explosiones,
                 std::vector<PowerUp>& items, std::vector<Boss>& jefes, PhysicsSpace& physics,
                 int& corazonesSpawneados)
{
    // Limpiar listas
    for (auto& bomba : bombas)
    {
        bomba.destruirFisica();
    }

    for (auto& enemigo : enemigos)
    {
        enemigo.destruir(physics);
    }

    for (auto& jefe : jefes)
    {
        jefe.destruir(physics);
    }

    bombas.clear();
    enemigos.clear();
    explosiones.clear();
    items.clear();
    jefes.clear();

    // Regenerar mapa
    mapa.inicializarGrid();
    mapa.generarFisicas(physics);
    llenarItemsIniciales(mapa, items, corazonesSpawneados);

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
        jefes.emplace_back(bosX, bosY, physics, "assets/images/Negro_spritesheet.png");
        std::cout << "NIVEL 3 INICIADO - ¡EL JEFE FINAL HA LLEGADO!" << std::endl;
        return;
    }

    // Spawnear enemigos normales desactivado temporalmente.
    int cantidadEnemigos = 0;


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

    /*
    for (int i = 0; i < cantidadEnemigos && i < static_cast<int>(spawnPoints.size()); i++)
    {
        enemigos.emplace_back(spawnPoints[i].first, spawnPoints[i].second, physics, "assets/images/Rojo_spritesheet.png");
    }
    */

    std::cout << "NIVEL " << nivel << " INICIADO - " << cantidadEnemigos << " ENEMIGOS" << std::endl;
}

// ============== FIN FUNCIÓN CARGAR NIVEL ==============

sf::Vector2f obtenerSpawnVersus(int personaje)
{
    switch (personaje)
    {
    case 1:
        return sf::Vector2f(96.0f, 96.0f);
    case 2:
        return sf::Vector2f(864.0f, 96.0f);
    case 3:
        return sf::Vector2f(96.0f, 736.0f);
    case 0:
    default:
        return sf::Vector2f(864.0f, 736.0f);
    }
}

std::string obtenerRutaGanadorVersus(int personaje)
{
    const std::string rutas[4] = {
        "assets/images/Ganador_verde.png",
        "assets/images/Ganador_rojo.png",
        "assets/images/Ganador_azul.png",
        "assets/images/Ganador_negro.png"
    };

    if (personaje < 0 || personaje >= 4)
    {
        return rutas[0];
    }

    return rutas[personaje];
}

std::string obtenerRutaBombaPersonaje(int personaje)
{
    const std::string rutas[4] = {
        "assets/images/Bomba_Verde.png",
        "assets/images/Bomba_Roja.png",
        "assets/images/Bomba_Azul.png",
        "assets/images/Bomba_Negra.png"
    };

    if (personaje < 0 || personaje >= 4)
    {
        return rutas[0];
    }

    return rutas[personaje];
}

sf::Color obtenerColorExplosionPersonaje(int personaje)
{
    switch (personaje)
    {
    case 1:
        return sf::Color(255, 50, 50, 200);
    case 2:
        return sf::Color(50, 150, 255, 200);
    case 3:
        return sf::Color(100, 0, 255, 200);
    case 0:
    default:
        return sf::Color(50, 255, 50, 200);
    }
}

sf::Vector2f obtenerCentroBody(b2BodyId bodyId)
{
    if (!b2Body_IsValid(bodyId))
    {
        return sf::Vector2f(-9999.0f, -9999.0f);
    }

    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return sf::Vector2f(pos.x * PIXELS_PER_METER, pos.y * PIXELS_PER_METER);
}

bool bombaEstaAlFrente(const sf::Vector2f& jugador, const sf::Vector2f& bomba, Direccion direccion)
{
    float dx = bomba.x - jugador.x;
    float dy = bomba.y - jugador.y;
    const float rangoFrontal = 74.0f;
    const float toleranciaLateral = 36.0f;

    if (direccion == ARRIBA)
        return dy < 0.0f && std::abs(dy) < rangoFrontal && std::abs(dx) < toleranciaLateral;
    if (direccion == ABAJO)
        return dy > 0.0f && std::abs(dy) < rangoFrontal && std::abs(dx) < toleranciaLateral;
    if (direccion == DERECHA)
        return dx > 0.0f && std::abs(dx) < rangoFrontal && std::abs(dy) < toleranciaLateral;
    return dx < 0.0f && std::abs(dx) < rangoFrontal && std::abs(dy) < toleranciaLateral;
}

void intentarPatearBombas(Knight& jugador, std::vector<Bomba>& bombas)
{
    if (!jugador.puedePatearBombas())
    {
        return;
    }

    sf::Vector2f posJugador = obtenerCentroBody(jugador.getBodyId());
    Direccion direccion = jugador.getDireccionActual();

    for (auto& bomba : bombas)
    {
        if (!bomba.isActiva() || !bomba.estaSolida() || bomba.estaEnMovimiento())
        {
            continue;
        }

        sf::Vector2f posBomba = obtenerCentroBody(bomba.getBodyId());
        if (bombaEstaAlFrente(posJugador, posBomba, direccion))
        {
            bomba.patear(direccion);
            break;
        }
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(960, 832), "Explosive-Knights - FASE 5");
    window.setFramerateLimit(60);

    // Máquina de estados y nivel (FASE 5)
    EstadoJuego estadoActual = MENU_PRINCIPAL;
    ModoJuego modoActual = ARCADE;
    int nivelActual = 1;

    // Instanciar Physics Engine
    PhysicsSpace physics;

    // Instanciar Mapa
    Mapa mapa;
    // Instanciar Knight (jugador)
    Knight knight(sf::Vector2f(96.0f, 96.0f), physics, "assets/images/Verde_spritesheet.png", 1);
    Knight knight2(sf::Vector2f(-500.0f, -500.0f), physics, "assets/images/Azul_spritesheet.png", 2);

    // Vector de bombas (FASE 2)
    std::vector<Bomba> listaBombas;

    // Vector de explosiones (FASE 3)
    std::vector<Explosion> listaExplosiones;

    // Vector de power-ups
    std::vector<PowerUp> listaItems;
    std::vector<float> temporizadoresRespawnItems;
    int corazonesSpawneados = 0;

    // Vector de enemigos (FASE 4)
    std::vector<Enemigo> listaEnemigos;

    // Vector de jefes (FASE 6)
    std::vector<Boss> listaJefes;

    Menu menu;
    menu.init();
    SeleccionPersonaje pantallaSeleccion;
    pantallaSeleccion.init();
    int modoSeleccionado = 0;
    int personajeSeleccionadoP1 = 0;
    int personajeSeleccionadoP2 = 2;
    int ganadorVersus = 0;
    int personajeGanadorVersus = 0;
    bool reinicioRondaPendiente = false;
    int perdedorRondaPendiente = 0;
    sf::Texture texturaGanadorVersus;
    sf::Sprite spriteGanadorVersus;
    bool texturaGanadorVersusCargada = false;

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

    sf::Text textoVictoriaVersus;
    if (fuenteCargada)
    {
        textoVictoriaVersus.setFont(fuente);
        textoVictoriaVersus.setCharacterSize(54);
        textoVictoriaVersus.setFillColor(sf::Color::Yellow);
        textoVictoriaVersus.setOutlineColor(sf::Color::Black);
        textoVictoriaVersus.setOutlineThickness(4.0f);
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

    sf::Text textoVolverMenu;
    if (fuenteCargada)
    {
        textoVolverMenu.setFont(fuente);
        textoVolverMenu.setCharacterSize(24);
        textoVolverMenu.setFillColor(sf::Color::White);
        textoVolverMenu.setOutlineColor(sf::Color::Black);
        textoVolverMenu.setOutlineThickness(2.0f);
        textoVolverMenu.setString("Presiona ENTER para volver al menu");
        textoVolverMenu.setPosition(960.0f / 2.0f - textoVolverMenu.getLocalBounds().width / 2.0f, 760.0f);
    }

    const float timeStep = 1.0f / 60.0f;  // 60 FPS

    auto volverAlMenuPrincipal = [&]()
    {
        for (auto& bomba : listaBombas)
        {
            bomba.destruirFisica();
        }

        for (auto& enemigo : listaEnemigos)
        {
            enemigo.destruir(physics);
        }

        for (auto& jefe : listaJefes)
        {
            jefe.destruir(physics);
        }

        listaBombas.clear();
        listaEnemigos.clear();
        listaExplosiones.clear();
        listaItems.clear();
        temporizadoresRespawnItems.clear();
        listaJefes.clear();

        mapa.cargarFondo("assets/images/Mapa.png");
        mapa.inicializarGrid();
        mapa.generarFisicas(physics);

        knight.reiniciar(96.0f, 96.0f);
        knight2.reiniciar(-500.0f, -500.0f);
        modoActual = ARCADE;
        nivelActual = 1;
        ganadorVersus = 0;
        personajeGanadorVersus = 0;
        reinicioRondaPendiente = false;
        perdedorRondaPendiente = 0;
        corazonesSpawneados = 0;
        texturaGanadorVersusCargada = false;
        pantallaSeleccion.setModoMultijugador(false);
        estadoActual = MENU_PRINCIPAL;
    };

    auto cargarVictoriaVersus = [&](int ganador)
    {
        ganadorVersus = ganador;
        personajeGanadorVersus = (ganador == 1) ? personajeSeleccionadoP1 : personajeSeleccionadoP2;
        texturaGanadorVersusCargada = texturaGanadorVersus.loadFromFile(obtenerRutaGanadorVersus(personajeGanadorVersus));
        if (texturaGanadorVersusCargada)
        {
            spriteGanadorVersus.setTexture(texturaGanadorVersus, true);
            sf::FloatRect bounds = spriteGanadorVersus.getLocalBounds();
            spriteGanadorVersus.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
            spriteGanadorVersus.setPosition(480.0f, 430.0f);
        }

        estadoActual = VICTORIA_VERSUS;
        std::cout << "PLAYER " << ganador << " GANA" << std::endl;
    };

    auto reiniciarRondaVersus = [&]()
    {
        for (auto& bomba : listaBombas)
        {
            bomba.destruirFisica();
        }

        for (auto& enemigo : listaEnemigos)
        {
            enemigo.destruir(physics);
        }

        for (auto& jefe : listaJefes)
        {
            jefe.destruir(physics);
        }

        listaBombas.clear();
        listaEnemigos.clear();
        listaExplosiones.clear();
        listaItems.clear();
        temporizadoresRespawnItems.clear();
        listaJefes.clear();

        mapa.cargarFondo("assets/images/Mapa_Versus.png");
        mapa.inicializarGrid();
        mapa.generarFisicas(physics);
        llenarItemsIniciales(mapa, listaItems, corazonesSpawneados);

        sf::Vector2f spawnP1 = obtenerSpawnVersus(personajeSeleccionadoP1);
        sf::Vector2f spawnP2 = obtenerSpawnVersus(personajeSeleccionadoP2);
        knight.reiniciarRonda(spawnP1.x, spawnP1.y);
        knight2.reiniciarRonda(spawnP2.x, spawnP2.y);
        reinicioRondaPendiente = false;
        perdedorRondaPendiente = 0;
        estadoActual = JUGANDO;
        std::cout << "RONDA VERSUS REINICIADA" << std::endl;
    };

    auto resolverDanoVersus = [&](int jugadorPerdedor)
    {
        if (jugadorPerdedor == 1)
        {
            if (knight.getVidas() <= 0)
            {
                cargarVictoriaVersus(2);
                return;
            }
        }
        else if (knight2.getVidas() <= 0)
        {
            cargarVictoriaVersus(1);
            return;
        }

        reiniciarRondaVersus();
    };

    while (window.isOpen())
    {
        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (estadoActual == MENU_PRINCIPAL)
            {
                menu.handleInput(event);
                if (menu.seleccionConfirmada())
                {
                    int opcion = menu.getOpcionSeleccionada();
                    menu.limpiarConfirmacion();

                    if (opcion == 0)
                    {
                        modoSeleccionado = opcion;
                        modoActual = ARCADE;
                        pantallaSeleccion.setModoMultijugador(false);
                        estadoActual = SELECCION_PERSONAJE;
                    }
                    else if (opcion == 1)
                    {
                        modoSeleccionado = opcion;
                        modoActual = MULTIJUGADOR;
                        pantallaSeleccion.setModoMultijugador(true);
                        estadoActual = SELECCION_PERSONAJE;
                    }
                    else if (opcion == 2)
                    {
                        window.close();
                    }
                }

                continue;
            }

            if (estadoActual == SELECCION_PERSONAJE)
            {
                pantallaSeleccion.handleInput(event);
                if (pantallaSeleccion.seleccionConfirmada())
                {
                    int personajeP1 = pantallaSeleccion.getPersonajeSeleccionadoP1();
                    int personajeP2 = pantallaSeleccion.getPersonajeSeleccionadoP2();
                    personajeSeleccionadoP1 = personajeP1;
                    personajeSeleccionadoP2 = personajeP2;

                    knight.cambiarSprite(pantallaSeleccion.getRutaTexturaPersonaje(personajeP1));
                    knight2.cambiarSprite(pantallaSeleccion.getRutaTexturaPersonaje(personajeP2));
                    knight.configurarBomba(obtenerRutaBombaPersonaje(personajeP1));
                    knight2.configurarBomba(obtenerRutaBombaPersonaje(personajeP2));
                    pantallaSeleccion.limpiarConfirmacion();
                    std::cout << "P1 seleccionado: " << personajeP1 << " | P2 seleccionado: " << personajeP2 << std::endl;

                    if (modoActual == ARCADE)
                    {
                        estadoActual = JUGANDO;
                        nivelActual = 1;
                        mapa.cargarFondo("assets/images/Mapa.png");
                        knight.reiniciar(96.0f, 96.0f);
                        knight2.reiniciar(-500.0f, -500.0f);
                        temporizadoresRespawnItems.clear();
                        cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics, corazonesSpawneados);
                    }
                    else if (modoActual == MULTIJUGADOR)
                    {
                        for (auto& bomba : listaBombas)
                        {
                            bomba.destruirFisica();
                        }

                        for (auto& enemigo : listaEnemigos)
                        {
                            enemigo.destruir(physics);
                        }

                        for (auto& jefe : listaJefes)
                        {
                            jefe.destruir(physics);
                        }

                        listaBombas.clear();
                        listaEnemigos.clear();
                        listaExplosiones.clear();
                        listaItems.clear();
                        temporizadoresRespawnItems.clear();
                        listaJefes.clear();

                        mapa.cargarFondo("assets/images/Mapa_Versus.png");
                        mapa.inicializarGrid();
                        mapa.generarFisicas(physics);
                        llenarItemsIniciales(mapa, listaItems, corazonesSpawneados);

                        sf::Vector2f spawnP1 = obtenerSpawnVersus(personajeP1);
                        sf::Vector2f spawnP2 = obtenerSpawnVersus(personajeP2);
                        knight.reiniciar(spawnP1.x, spawnP1.y);
                        knight2.reiniciar(spawnP2.x, spawnP2.y);
                        nivelActual = 1;
                        estadoActual = JUGANDO;
                    }
                }

                continue;
            }

            // Manejar input de Espacio para plantar bomba (FASE 2)
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Space && estadoActual == JUGANDO)
                {
                    knight.plantarBomba(mapa, listaBombas, physics, 1);
                }
                if (event.key.code == sf::Keyboard::Numpad0 && estadoActual == JUGANDO && modoActual == MULTIJUGADOR)
                {
                    knight2.plantarBomba(mapa, listaBombas, physics, 2);
                }
                if (event.key.code == sf::Keyboard::Return && estadoActual == VICTORIA_VERSUS)
                {
                    volverAlMenuPrincipal();
                }
                // Reiniciar con Enter si es GAME_OVER o VICTORIA (FASE 5)
                if (event.key.code == sf::Keyboard::Return && (estadoActual == GAME_OVER || estadoActual == VICTORIA))
                {
                    estadoActual = JUGANDO;
                    nivelActual = 1;
                    if (modoActual == MULTIJUGADOR)
                    {
                        for (auto& bomba : listaBombas)
                        {
                            bomba.destruirFisica();
                        }

                        for (auto& enemigo : listaEnemigos)
                        {
                            enemigo.destruir(physics);
                        }

                        for (auto& jefe : listaJefes)
                        {
                            jefe.destruir(physics);
                        }

                        listaBombas.clear();
                        listaEnemigos.clear();
                        listaExplosiones.clear();
                        listaItems.clear();
                        temporizadoresRespawnItems.clear();
                        listaJefes.clear();

                        mapa.cargarFondo("assets/images/Mapa_Versus.png");
                        mapa.inicializarGrid();
                        mapa.generarFisicas(physics);
                        llenarItemsIniciales(mapa, listaItems, corazonesSpawneados);
                        sf::Vector2f spawnP1 = obtenerSpawnVersus(personajeSeleccionadoP1);
                        sf::Vector2f spawnP2 = obtenerSpawnVersus(personajeSeleccionadoP2);
                        knight.reiniciar(spawnP1.x, spawnP1.y);
                        knight2.reiniciar(spawnP2.x, spawnP2.y);
                    }
                    else
                    {
                        mapa.cargarFondo("assets/images/Mapa.png");
                        knight.reiniciar(96.0f, 96.0f);
                        knight2.reiniciar(-500.0f, -500.0f);
                        temporizadoresRespawnItems.clear();
                        cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics, corazonesSpawneados);
                    }
                }
            }
        }

        if (estadoActual == MENU_PRINCIPAL)
        {
            window.clear(sf::Color(24, 28, 38));
            menu.draw(window);
            window.display();
            continue;
        }

        if (estadoActual == SELECCION_PERSONAJE)
        {
            pantallaSeleccion.update();
            window.clear(sf::Color(22, 24, 34));
            pantallaSeleccion.draw(window);
            window.display();
            continue;
        }

        // INPUT
        knight.handleInput();
        if (modoActual == MULTIJUGADOR)
        {
            knight2.handleInput();
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
        knight.update(timeStep);
        if (modoActual == MULTIJUGADOR)
        {
            knight2.update(timeStep);
            intentarPatearBombas(knight, listaBombas);
            intentarPatearBombas(knight2, listaBombas);
        }

        // UPDATE BOMBAS (FASE 2) + EXPLOSIONES (FASE 3)
        std::vector<b2BodyId> jugadoresBodyId = {knight.getBodyId()};
        if (modoActual == MULTIJUGADOR)
        {
            jugadoresBodyId.push_back(knight2.getBodyId());
        }

        for (auto& bomba : listaBombas)
        {
            if (bomba.update(physics, jugadoresBodyId))  // Si retorna true = explotó
            {
                // Crear explosión con el rango del jugador que puso la bomba
                int personajeExplosion = personajeSeleccionadoP1;
                if (bomba.getPropietario() == 2)
                {
                    personajeExplosion = personajeSeleccionadoP2;
                }

                Explosion explosion(
                    bomba.getFila(),
                    bomba.getColumna(),
                    mapa,
                    bomba.getRangoFuego(),
                    &listaItems,
                    obtenerColorExplosionPersonaje(personajeExplosion)
                );
                listaExplosiones.push_back(explosion);
            }
            bomba.actualizarMovimientoPateado(mapa);
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

        mapa.update(timeStep);

        auto respawnItem = temporizadoresRespawnItems.begin();
        while (modoActual != MULTIJUGADOR && respawnItem != temporizadoresRespawnItems.end())
        {
            *respawnItem -= timeStep;
            if (*respawnItem <= 0.0f)
            {
                colocarItemOcultoAleatorio(mapa, listaItems, corazonesSpawneados);
                respawnItem = temporizadoresRespawnItems.erase(respawnItem);
            }
            else
            {
                ++respawnItem;
            }
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

        if (estadoActual == JUGANDO &&
            modoActual == MULTIJUGADOR &&
            reinicioRondaPendiente &&
            listaExplosiones.empty())
        {
            resolverDanoVersus(perdedorRondaPendiente);
        }

        // UPDATE ENEMIGOS (FASE 4)
        for (auto& enemigo : listaEnemigos)
        {
            if (enemigo.isVivo())
            {
                enemigo.update(mapa, physics);
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
        for (auto& boss : listaJefes)
        {
            boss.update(mapa, physics);
        }

        // Daño a Boss por explosiones (FASE 6)
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

        bool danoVersusP1 = false;
        bool danoVersusP2 = false;

        // Ataque de Boss al jugador (FASE 6)
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
                if (knight.recibirDano(physics, modoActual == MULTIJUGADOR) && modoActual == MULTIJUGADOR)
                {
                    danoVersusP1 = true;
                }
            }
        }

        // LÓGICA DE VICTORIA/DERROTA (FASE 5 y 6)
        if (estadoActual == JUGANDO)
        {
            // Verificar si el jugador murió
            if (false && modoActual == MULTIJUGADOR && knight.getVidas() <= 0)
            {
                ganadorVersus = 2;
                personajeGanadorVersus = personajeSeleccionadoP2;
                texturaGanadorVersusCargada = texturaGanadorVersus.loadFromFile(obtenerRutaGanadorVersus(personajeGanadorVersus));
                if (texturaGanadorVersusCargada)
                {
                    spriteGanadorVersus.setTexture(texturaGanadorVersus, true);
                    sf::FloatRect bounds = spriteGanadorVersus.getLocalBounds();
                    spriteGanadorVersus.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
                    spriteGanadorVersus.setPosition(480.0f, 430.0f);
                }
                estadoActual = VICTORIA_VERSUS;
                std::cout << "PLAYER 2 GANA" << std::endl;
            }
            else if (false && modoActual == MULTIJUGADOR && knight2.getVidas() <= 0)
            {
                ganadorVersus = 1;
                personajeGanadorVersus = personajeSeleccionadoP1;
                texturaGanadorVersusCargada = texturaGanadorVersus.loadFromFile(obtenerRutaGanadorVersus(personajeGanadorVersus));
                if (texturaGanadorVersusCargada)
                {
                    spriteGanadorVersus.setTexture(texturaGanadorVersus, true);
                    sf::FloatRect bounds = spriteGanadorVersus.getLocalBounds();
                    spriteGanadorVersus.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
                    spriteGanadorVersus.setPosition(480.0f, 430.0f);
                }
                estadoActual = VICTORIA_VERSUS;
                std::cout << "PLAYER 1 GANA" << std::endl;
            }
            else if (modoActual == ARCADE && knight.getVidas() <= 0)
            {
                estadoActual = GAME_OVER;
                std::cout << "GAME OVER - Presiona ENTER para reiniciar" << std::endl;
            }
            // FASE 6: Si estamos en nivel 3, verificar si el Boss fue derrotado
            else if (modoActual == ARCADE && nivelActual == 3 && listaJefes.empty())
            {
                estadoActual = VICTORIA;
                std::cout << "¡VICTORIA! ¡Derrotaste al Jefe Final! - Presiona ENTER para reiniciar" << std::endl;
            }
            // Verificar si ganó el nivel (todos los enemigos muertos) - Niveles 1 y 2
            else if (false && listaEnemigos.empty() && nivelActual < 3)
            {
                nivelActual++;
                std::cout << "¡NIVEL COMPLETADO! Avanzando a NIVEL " << nivelActual << std::endl;
                cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics, corazonesSpawneados);
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
        sf::FloatRect knightDamageRect(
            pixelXKnight - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
            pixelYKnight - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
            KNIGHT_DAMAGE_HITBOX_SIZE,
            KNIGHT_DAMAGE_HITBOX_SIZE
        );
        sf::FloatRect knight2DamageRect;
        int filaKnight2 = -1;
        int colKnight2 = -1;
        if (modoActual == MULTIJUGADOR)
        {
            b2Vec2 posKnight2 = b2Body_GetPosition(knight2.getBodyId());
            float pixelXKnight2 = posKnight2.x * PIXELS_PER_METER;
            float pixelYKnight2 = posKnight2.y * PIXELS_PER_METER;
            filaKnight2 = static_cast<int>(pixelYKnight2 / 64.0f);
            colKnight2 = static_cast<int>(pixelXKnight2 / 64.0f);
            knight2DamageRect = sf::FloatRect(
                pixelXKnight2 - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
                pixelYKnight2 - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
                KNIGHT_DAMAGE_HITBOX_SIZE,
                KNIGHT_DAMAGE_HITBOX_SIZE
            );
        }

        // Recopilar items
        for (auto& item : listaItems)
        {
            if (item.isActivo() && item.estaVisible() && item.getFila() == filaKnight && item.getColumna() == colKnight)
            {
                knight.recolectarItem(item.getTipo(), modoActual == MULTIJUGADOR);
                item.desactivar();
                if (modoActual != MULTIJUGADOR)
                {
                    temporizadoresRespawnItems.push_back(TIEMPO_RESPAWN_ITEM);
                }
            }
            else if (modoActual == MULTIJUGADOR &&
                     item.isActivo() &&
                     item.estaVisible() &&
                     item.getFila() == filaKnight2 &&
                     item.getColumna() == colKnight2)
            {
                knight2.recolectarItem(item.getTipo(), true);
                item.desactivar();
                if (modoActual != MULTIJUGADOR)
                {
                    temporizadoresRespawnItems.push_back(TIEMPO_RESPAWN_ITEM);
                }
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
                    sf::FloatRect fireDamageRect(
                        celda.y * 64.0f + FIRE_DAMAGE_MARGIN,
                        celda.x * 64.0f + FIRE_DAMAGE_MARGIN,
                        64.0f - FIRE_DAMAGE_MARGIN * 2.0f,
                        64.0f - FIRE_DAMAGE_MARGIN * 2.0f
                    );

                    if (fireDamageRect.intersects(knightDamageRect))
                    {
                        if (knight.recibirDano(physics, modoActual == MULTIJUGADOR) && modoActual == MULTIJUGADOR)
                        {
                            danoVersusP1 = true;
                        }
                        break;
                    }
                }

                // Daño por fuego a P2
                if (modoActual == MULTIJUGADOR)
                {
                    for (const auto& celda : celdasAfectadas)
                    {
                        sf::FloatRect fireDamageRect(
                            celda.y * 64.0f + FIRE_DAMAGE_MARGIN,
                            celda.x * 64.0f + FIRE_DAMAGE_MARGIN,
                            64.0f - FIRE_DAMAGE_MARGIN * 2.0f,
                            64.0f - FIRE_DAMAGE_MARGIN * 2.0f
                        );

                        if (fireDamageRect.intersects(knight2DamageRect))
                        {
                            if (knight2.recibirDano(physics, true))
                            {
                                danoVersusP2 = true;
                            }
                            break;
                        }
                    }
                }

                // Daño por fuego a enemigos (FASE 4)
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
                    if (knight.recibirDano(physics, modoActual == MULTIJUGADOR) && modoActual == MULTIJUGADOR)
                    {
                        danoVersusP1 = true;
                    }
                }
            }
        }

        if (estadoActual == JUGANDO && modoActual == MULTIJUGADOR && (danoVersusP1 || danoVersusP2))
        {
            reinicioRondaPendiente = true;
            perdedorRondaPendiente = danoVersusP1 ? 1 : 2;
        }

        // ACTUALIZAR HUD (FASE 5)
        if (fuenteCargada)
        {
            std::string hudText;
            if (modoActual == MULTIJUGADOR)
            {
                hudText = "VERSUS | P1 VIDAS: " + std::to_string(knight.getVidas()) +
                          " | P2 VIDAS: " + std::to_string(knight2.getVidas());
            }
            else
            {
                hudText = "NIVEL: " + std::to_string(nivelActual) + 
                          " | VIDAS: " + std::to_string(knight.getVidas()) + 
                          " | ENEMIGOS: " + std::to_string(listaEnemigos.size());
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
            dibujarItemsActivosHUD(window, knight, fuente, "P1", 10.0f, 38.0f);
            if (modoActual == MULTIJUGADOR)
            {
                dibujarItemsActivosHUD(window, knight2, fuente, "P2", 10.0f, 78.0f);
            }
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

        if (estadoActual == VICTORIA_VERSUS)
        {
            sf::RectangleShape overlay(sf::Vector2f(960.0f, 832.0f));
            overlay.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(overlay);

            if (texturaGanadorVersusCargada)
            {
                window.draw(spriteGanadorVersus);
            }

            if (fuenteCargada)
            {
                textoVictoriaVersus.setString(std::string("\xC2\xA1PLAYER ") + std::to_string(ganadorVersus) + " GANA!");
                textoVictoriaVersus.setPosition(
                    480.0f - textoVictoriaVersus.getLocalBounds().width / 2.0f,
                    120.0f
                );
                window.draw(textoVictoriaVersus);
                window.draw(textoVolverMenu);
            }
        }

        window.display();
    }

    return 0;
}
