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
#include <iomanip>
#include <sstream>
#include <utility>
#include "Mapa.hpp"
#include "Menu.hpp"
#include "SeleccionPersonaje.hpp"
#include "MenuArcadePlayers.hpp"
#include "Puerta.hpp"
#include "GestorArcade.hpp"

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
    MENU_ARCADE_PLAYERS,
    SELECCION_PERSONAJE,
    SELECCION_ARCADE,
    JUGANDO,
    JUGANDO_ARCADE,
    TRANSICION_NIVEL,
    GAME_OVER,
    VICTORIA,
    VICTORIA_ARCADE,
    VICTORIA_VERSUS,
    EMPATE_VERSUS
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
const float ENTITY_COLLISION_SIZE = TAMANO_TILE * 0.95f;
const float KNIGHT_COLLISION_SIZE = ENTITY_COLLISION_SIZE;
const float KNIGHT_COLLISION_RADIUS = TAMANO_TILE * 0.49f;
const float BOMB_COLLISION_SIZE = ENTITY_COLLISION_SIZE;
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
const int MAX_CORAZONES_POR_MAPA = 1;
const int PROBABILIDAD_CORAZON = 8;
const float DURACION_ITEM_TEMPORAL = 10.0f;
const float DURACION_ESCUDO_ITEM = 15.0f;
const float TIEMPO_RESPAWN_ITEM = 20.0f;
const float INCREMENTO_VELOCIDAD_VERSUS = 1.0f;
const float VELOCIDAD_BOMBA_PATEADA = 8.0f;
const float DURACION_RONDA_VERSUS = 180.0f;
const float INTERVALO_CAIDA_MUERTE_SUBITA = 0.5f;

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
        float scaleY = MAP_CELL_SIZE / static_cast<float>(frameHeight);
        float scaleX = scaleY;
        sprite.setScale(scaleX, scaleY);
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
    ITEM_FLAMA = 6,
    ITEM_TOTAL = 7
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
            item.setScale(
                MAP_CELL_SIZE / static_cast<float>(size.x),
                MAP_CELL_SIZE / static_cast<float>(size.y)
            );
            item.setPosition(posX, posY);
            window.draw(item);
            return;
        }

        if (activo)
        {
            sf::CircleShape item(MAP_CELL_SIZE / 2.0f);
            item.setOrigin(MAP_CELL_SIZE / 2.0f, MAP_CELL_SIZE / 2.0f);
            item.setPosition(posX, posY);

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
            "Patear",
            "Flama"
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
            "assets/images/item_patear.png",
            "assets/images/item_flama.png"
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
std::array<bool, ITEM_TOTAL> PowerUp::texturasCargadas = {false, false, false, false, false, false, false};
std::array<bool, ITEM_TOTAL> PowerUp::texturasIntentadas = {false, false, false, false, false, false, false};

// ============== FIN POWER-UP ==============

#include "Mapa.cpp"
#include "Menu.cpp"
#include "SeleccionPersonaje.cpp"
#include "MenuArcadePlayers.cpp"
#include "Puerta.cpp"

// ============== EXPLOSIÓN (FASE 3: Explosiones y Destrucción) ==============
class Explosion
{
public:
    Explosion(int centroFila, int centroCol, Mapa& mapa, int rango = 2,
              std::vector<PowerUp>* pItems = nullptr,
              sf::Color colorBase = sf::Color(255, 165, 0, 200))
        : centroFila(centroFila), centroCol(centroCol), colorBase(colorBase),
          activa(true), pListaItems(pItems)
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
        if (!activa)
        {
            return;
        }

        float progreso = temporizador.getElapsedTime().asSeconds() / DURACION_EXPLOSION;
        if (progreso > 1.0f)
        {
            progreso = 1.0f;
        }

        for (const auto& celda : celdasAfectadas)
        {
            dibujarCeldaFuego(window, celda.x, celda.y, progreso);
        }
    }

    bool isActiva() const { return activa; }

    const std::vector<sf::Vector2i>& getCeldasAfectadas() const { return celdasAfectadas; }

private:
    static constexpr float DURACION_EXPLOSION = Mapa::DURACION_DESTRUCCION_BLOQUE;

    bool existeCelda(int fila, int col) const
    {
        return std::find(celdasAfectadas.begin(), celdasAfectadas.end(), sf::Vector2i(fila, col)) != celdasAfectadas.end();
    }

    sf::Color conAlpha(sf::Color color, float alpha) const
    {
        alpha = std::max(0.0f, std::min(255.0f, alpha));
        color.a = static_cast<sf::Uint8>(alpha);
        return color;
    }

    sf::Color mezclarColor(sf::Color a, sf::Color b, float t) const
    {
        t = std::max(0.0f, std::min(1.0f, t));
        return sf::Color(
            static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
            static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
            static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
            static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
        );
    }

    void dibujarCirculo(sf::RenderWindow& window, sf::Vector2f centro, float radio, sf::Color color) const
    {
        sf::CircleShape circulo(radio, 28);
        circulo.setOrigin(radio, radio);
        circulo.setPosition(centro);
        circulo.setFillColor(color);
        window.draw(circulo);
    }

    void dibujarRectCentro(sf::RenderWindow& window, sf::Vector2f centro, sf::Vector2f tamano, sf::Color color) const
    {
        sf::RectangleShape rect(tamano);
        rect.setOrigin(tamano.x / 2.0f, tamano.y / 2.0f);
        rect.setPosition(centro);
        rect.setFillColor(color);
        window.draw(rect);
    }

    void dibujarCapaFuego(sf::RenderWindow& window, sf::Vector2f centro, int dx, int dy,
                          bool esCentro, bool esPunta, float escala, sf::Color color) const
    {
        float largo = MAP_CELL_SIZE * escala;
        float grosor = (esCentro ? 58.0f : 34.0f) * escala;

        if (esCentro)
        {
            dibujarCirculo(window, centro, 30.0f * escala, color);
            dibujarRectCentro(window, centro, sf::Vector2f(58.0f * escala, 28.0f * escala), color);
            dibujarRectCentro(window, centro, sf::Vector2f(28.0f * escala, 58.0f * escala), color);
            return;
        }

        if (dx != 0)
        {
            dibujarRectCentro(window, centro, sf::Vector2f(largo, grosor), color);
            dibujarCirculo(window, centro, grosor * 0.5f, color);
            if (esPunta)
            {
                dibujarCirculo(window, sf::Vector2f(centro.x + dx * 20.0f * escala, centro.y), grosor * 0.72f, color);
            }
        }
        else
        {
            dibujarRectCentro(window, centro, sf::Vector2f(grosor, largo), color);
            dibujarCirculo(window, centro, grosor * 0.5f, color);
            if (esPunta)
            {
                dibujarCirculo(window, sf::Vector2f(centro.x, centro.y + dy * 20.0f * escala), grosor * 0.72f, color);
            }
        }
    }

    void dibujarCeldaFuego(sf::RenderWindow& window, int fila, int col, float progreso) const
    {
        int dx = col - centroCol;
        int dy = fila - centroFila;
        bool esCentro = (dx == 0 && dy == 0);
        int dirX = (dx > 0) - (dx < 0);
        int dirY = (dy > 0) - (dy < 0);
        bool esPunta = !esCentro && !existeCelda(fila + dirY, col + dirX);

        float entrada = std::min(1.0f, progreso / 0.18f);
        float salida = progreso > 0.68f ? (1.0f - progreso) / 0.32f : 1.0f;
        salida = std::max(0.0f, std::min(1.0f, salida));
        float pulso = 0.92f + 0.12f * std::sin(progreso * 3.14159265f * 3.0f);
        float escala = entrada * salida * pulso;
        float alpha = 235.0f * salida;

        sf::Vector2f centro(
            col * MAP_CELL_SIZE + MAP_CELL_SIZE / 2.0f,
            fila * MAP_CELL_SIZE + MAP_CELL_SIZE / 2.0f
        );

        sf::Color bordePersonaje = mezclarColor(sf::Color(255, 88, 16), colorBase, 0.18f);
        sf::Color exterior = conAlpha(bordePersonaje, alpha * 0.78f);
        sf::Color medio = conAlpha(sf::Color(255, 144, 24), alpha * 0.90f);
        sf::Color interior = conAlpha(sf::Color(255, 232, 84), alpha);
        sf::Color nucleo = conAlpha(sf::Color(255, 255, 230), alpha);

        dibujarCapaFuego(window, centro, dx, dy, esCentro, esPunta, escala * 1.06f, exterior);
        dibujarCapaFuego(window, centro, dx, dy, esCentro, esPunta, escala * 0.78f, medio);
        dibujarCapaFuego(window, centro, dx, dy, esCentro, esPunta, escala * 0.48f, interior);

        if (esCentro)
        {
            dibujarCirculo(window, centro, 11.0f * escala, nucleo);
        }
        else if (esPunta)
        {
            dibujarCirculo(window, sf::Vector2f(centro.x + dirX * 13.0f * escala, centro.y + dirY * 13.0f * escala),
                           8.0f * escala, nucleo);
        }
    }

    int centroFila;
    int centroCol;
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
          bodyId(b2_nullBodyId), shapeId(b2_nullShapeId), texturaCargada(false),
          explosionForzada(false), explotando(false)
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
                (BOMB_COLLISION_SIZE / 2.0f) / PIXELS_PER_METER,
                (BOMB_COLLISION_SIZE / 2.0f) / PIXELS_PER_METER
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
          texturaCargada(otra.texturaCargada), explosionForzada(otra.explosionForzada),
          explotando(otra.explotando), temporizador(std::move(otra.temporizador))
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
            explosionForzada = otra.explosionForzada;
            explotando = otra.explotando;
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

        if (activa && (explosionForzada || temporizador.getElapsedTime().asSeconds() >= 3.0f))
        {
            activa = false;
            explotando = true;
            
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
                float tiempo = temporizador.getElapsedTime().asSeconds();
                float alerta = std::max(0.0f, std::min(1.0f, (tiempo - 2.0f) / 1.0f));
                float pulso = 1.0f + (0.06f + alerta * 0.08f) * std::sin(tiempo * (8.0f + alerta * 8.0f));
                sf::Vector2u size = textura.getSize();
                sprite.setScale(
                    (MAP_CELL_SIZE / static_cast<float>(size.x)) * pulso,
                    (MAP_CELL_SIZE / static_cast<float>(size.y)) * pulso
                );
                sprite.setColor(sf::Color(
                    255,
                    static_cast<sf::Uint8>(255 - alerta * 35.0f),
                    static_cast<sf::Uint8>(255 - alerta * 35.0f),
                    255
                ));
                sprite.setPosition(posX, posY);
                window.draw(sprite);

                if (alerta > 0.05f)
                {
                    float chispaRadio = 3.0f + 3.0f * std::abs(std::sin(tiempo * 18.0f));
                    sf::CircleShape chispa(chispaRadio, 14);
                    chispa.setOrigin(chispaRadio, chispaRadio);
                    chispa.setFillColor(sf::Color(255, 230, 80, static_cast<sf::Uint8>(180.0f * alerta)));
                    chispa.setPosition(posX + 14.0f, posY - 22.0f);
                    window.draw(chispa);
                }

                return;
            }

            sf::CircleShape bomba(MAP_CELL_SIZE / 2.0f);
            bomba.setOrigin(MAP_CELL_SIZE / 2.0f, MAP_CELL_SIZE / 2.0f);
            bomba.setFillColor(sf::Color::Black);
            bomba.setPosition(posX, posY);
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
    bool estaEnCelda(int f, int c) const { return fila == f && columna == c; }
    bool estaExplotando() const { return explotando; }
    void forzarExplosion()
    {
        if (!activa || explotando)
        {
            return;
        }

        explosionForzada = true;
        explotando = true;
    }

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
        sprite.setScale(
            MAP_CELL_SIZE / static_cast<float>(size.x),
            MAP_CELL_SIZE / static_cast<float>(size.y)
        );
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
    bool explosionForzada;
    bool explotando;
    sf::Clock temporizador;
};

// ============== FIN BOMBA ==============

bool forzarBombasTocadasPorExplosion(const Explosion& explosion, std::vector<Bomba>& bombas)
{
    bool algunaForzada = false;
    const auto& celdas = explosion.getCeldasAfectadas();

    for (auto& bomba : bombas)
    {
        if (!bomba.isActiva() || bomba.estaExplotando())
        {
            continue;
        }

        for (const auto& celda : celdas)
        {
            if (bomba.estaEnCelda(celda.x, celda.y))
            {
                bomba.forzarExplosion();
                algunaForzada = true;
                break;
            }
        }
    }

    return algunaForzada;
}

void destruirItemsTocadosPorExplosion(const Explosion& explosion, std::vector<PowerUp>& items)
{
    const auto& celdas = explosion.getCeldasAfectadas();

    for (auto& item : items)
    {
        if (!item.isActivo() || !item.estaVisible())
        {
            continue;
        }

        for (const auto& celda : celdas)
        {
            if (item.estaEnCelda(celda.x, celda.y))
            {
                item.desactivar();
                break;
            }
        }
    }
}

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
          tiempoVelocidad(0.0f), tiempoPatear(0.0f), tiempoFlama(0.0f),
          bombasExtraRonda(0), velocidadExtraRonda(0.0f),
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
        actualizarTemporizador(tiempoBombaExtra, deltaTime);
        actualizarTemporizador(tiempoVelocidad, deltaTime);
        actualizarTemporizador(tiempoFantasma, deltaTime);
        actualizarTemporizador(tiempoPatear, deltaTime);
        actualizarTemporizador(tiempoFlama, deltaTime);

        maxBombas = 1;
        if (tiempoBombaExtra > 0.0f)
        {
            maxBombas = 2;
        }

        speed = speedOriginal;
        if (tiempoVelocidad > 0.0f)
        {
            speed = speedOriginal * 1.5f;
        }

        puedePatear = tiempoPatear > 0.0f;
        rangoFuego = tiempoFlama > 0.0f ? 2 : 1;
        setIgnorarDestructibles(tiempoFantasma > 0.0f);

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
        (void)modoVersus;

        if (tipo == ITEM_BOMBA_EXTRA)
        {
            tiempoBombaExtra = DURACION_ITEM_TEMPORAL;
            maxBombas = 2;
        }
        else if (tipo == ITEM_ESCUDO)
        {
            // Fuego: aumentar rango
            tiempoEscudo = DURACION_ESCUDO_ITEM;
        }
        else if (tipo == ITEM_FANTASMA)
        {
            // Bomba Extra: aumentar máximo de bombas
            if (false)
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
            if (false)
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
            tiempoPatear = DURACION_ITEM_TEMPORAL;
            puedePatear = true;
        }
        else if (tipo == ITEM_FLAMA)
        {
            tiempoFlama = DURACION_ITEM_TEMPORAL;
            rangoFuego = 2;
        }
    }

    bool morir(PhysicsSpace& physics, bool conservarMejoras = false)
    {
        return recibirDano(physics, conservarMejoras);
    }

    bool recibirDano(PhysicsSpace& physics, bool conservarMejoras = false)
    {
        (void)physics;
        if (vidas <= 0)
        {
            return false;
        }

        if (tiempoEscudo > 0.0f)
        {
            tiempoEscudo = 0.0f;
            return false;
        }

        if (tiempoInvulnerable > 0.0f)
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
            return tiempoBombaExtra;
        if (tipo == ITEM_ESCUDO)
            return tiempoEscudo;
        if (tipo == ITEM_FANTASMA)
            return tiempoFantasma;
        if (tipo == ITEM_VELOCIDAD)
            return tiempoVelocidad;
        if (tipo == ITEM_PATEAR)
            return tiempoPatear;
        if (tipo == ITEM_FLAMA)
            return tiempoFlama;
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
        tiempoPatear = 0.0f;
        tiempoFlama = 0.0f;
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
    float tiempoPatear;
    float tiempoFlama;
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
    int roll = rand() % 100;
    if (roll < PROBABILIDAD_CORAZON && corazonesSpawneados < MAX_CORAZONES_POR_MAPA)
    {
        corazonesSpawneados++;
        return ITEM_VIDA;
    }

    const int itemsComunes[] = {
        ITEM_BOMBA_EXTRA,
        ITEM_FLAMA,
        ITEM_VELOCIDAD,
        ITEM_ESCUDO,
        ITEM_FANTASMA,
        ITEM_PATEAR
    };

    return itemsComunes[rand() % 6];
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

sf::Color obtenerColorItemHUD(int tipo)
{
    switch (tipo)
    {
    case ITEM_BOMBA_EXTRA:
        return sf::Color(255, 184, 42);
    case ITEM_ESCUDO:
        return sf::Color(52, 190, 255);
    case ITEM_FANTASMA:
        return sf::Color(178, 126, 255);
    case ITEM_VELOCIDAD:
        return sf::Color(66, 225, 255);
    case ITEM_PATEAR:
        return sf::Color(245, 169, 55);
    case ITEM_FLAMA:
        return sf::Color(255, 82, 24);
    default:
        return sf::Color::White;
    }
}

float obtenerProgresoItemHUD(int tipo, float tiempo)
{
    if (tiempo <= 0.0f)
    {
        return 0.0f;
    }

    float duracion = tipo == ITEM_ESCUDO ? DURACION_ESCUDO_ITEM : DURACION_ITEM_TEMPORAL;
    return std::min(1.0f, tiempo / duracion);
}

void dibujarBombaDoradaHUD(sf::RenderWindow& window, float x, float y, float escala = 1.0f)
{
    sf::CircleShape cuerpo(7.0f * escala);
    cuerpo.setOrigin(7.0f * escala, 7.0f * escala);
    cuerpo.setPosition(x, y);
    cuerpo.setFillColor(sf::Color(246, 184, 41));
    cuerpo.setOutlineColor(sf::Color(100, 61, 10));
    cuerpo.setOutlineThickness(2.0f * escala);
    window.draw(cuerpo);

    sf::RectangleShape brillo(sf::Vector2f(5.0f * escala, 4.0f * escala));
    brillo.setOrigin(2.5f * escala, 2.0f * escala);
    brillo.setPosition(x - 3.0f * escala, y - 4.0f * escala);
    brillo.setFillColor(sf::Color(255, 244, 130));
    window.draw(brillo);

    sf::RectangleShape mecha(sf::Vector2f(4.0f * escala, 8.0f * escala));
    mecha.setOrigin(2.0f * escala, 7.0f * escala);
    mecha.setPosition(x + 4.0f * escala, y - 8.0f * escala);
    mecha.setRotation(-35.0f);
    mecha.setFillColor(sf::Color(128, 73, 18));
    window.draw(mecha);

    sf::CircleShape chispa(3.0f * escala, 5);
    chispa.setOrigin(3.0f * escala, 3.0f * escala);
    chispa.setPosition(x + 8.0f * escala, y - 13.0f * escala);
    chispa.setFillColor(sf::Color(255, 238, 95));
    window.draw(chispa);
}

void dibujarCorazonesVidaHUD(sf::RenderWindow& window, int vidas, float x, float y)
{
    sf::Texture* texturaVida = PowerUp::obtenerTextura(ITEM_VIDA);
    int vidasADibujar = std::max(0, vidas);

    for (int i = 0; i < vidasADibujar; i++)
    {
        float posX = x + static_cast<float>(i % 4) * 18.0f;
        float posY = y + static_cast<float>(i / 4) * 16.0f;

        if (texturaVida != nullptr)
        {
            sf::Sprite corazon;
            corazon.setTexture(*texturaVida);
            sf::Vector2u size = texturaVida->getSize();
            corazon.setOrigin(size.x / 2.0f, size.y / 2.0f);
            float escala = 18.0f / static_cast<float>(std::max(size.x, size.y));
            corazon.setScale(escala, escala);
            corazon.setPosition(posX, posY);
            window.draw(corazon);
        }
        else
        {
            sf::CircleShape corazonFallback(6.0f);
            corazonFallback.setOrigin(6.0f, 6.0f);
            corazonFallback.setFillColor(sf::Color(240, 40, 60));
            corazonFallback.setPosition(posX, posY);
            window.draw(corazonFallback);
        }
    }
}

void dibujarBarrasItemsHUD(sf::RenderWindow& window, const Knight& knight, float x, float y)
{
    const int tiposTemporales[6] = {
        ITEM_BOMBA_EXTRA,
        ITEM_ESCUDO,
        ITEM_FANTASMA,
        ITEM_VELOCIDAD,
        ITEM_PATEAR,
        ITEM_FLAMA
    };

    float offsetY = 0.0f;

    for (int tipo : tiposTemporales)
    {
        float tiempo = knight.getTiempoItem(tipo);
        if (tiempo <= 0.0f)
        {
            continue;
        }

        sf::RectangleShape fondo(sf::Vector2f(126.0f, 16.0f));
        fondo.setFillColor(sf::Color(0, 0, 0, 150));
        fondo.setOutlineColor(sf::Color(255, 255, 255, 80));
        fondo.setOutlineThickness(1.0f);
        fondo.setPosition(x, y + offsetY);
        window.draw(fondo);

        sf::Texture* textura = PowerUp::obtenerTextura(tipo);
        if (textura != nullptr)
        {
            sf::Sprite icono;
            icono.setTexture(*textura);
            sf::Vector2u size = textura->getSize();
            icono.setOrigin(size.x / 2.0f, size.y / 2.0f);
            float escala = 18.0f / static_cast<float>(std::max(size.x, size.y));
            icono.setScale(escala, escala);
            icono.setPosition(x + 10.0f, y + offsetY + 8.0f);
            window.draw(icono);
        }

        float progreso = obtenerProgresoItemHUD(tipo, tiempo);
        sf::RectangleShape barra(sf::Vector2f(96.0f * progreso, 8.0f));
        barra.setFillColor(obtenerColorItemHUD(tipo));
        barra.setPosition(x + 24.0f, y + offsetY + 4.0f);
        window.draw(barra);

        offsetY += 19.0f;
    }
}

void dibujarPanelJugadorHUD(sf::RenderWindow& window, const Knight& knight, int personaje,
                            sf::Texture* texturaMarco, int rondasGanadas,
                            const std::string& etiqueta, sf::Font* fuente, float x, float y,
                            bool barrasALaIzquierda = false)
{
    sf::RectangleShape sombra(sf::Vector2f(76.0f, 76.0f));
    sombra.setPosition(x + 4.0f, y + 5.0f);
    sombra.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(sombra);

    if (texturaMarco != nullptr)
    {
        sf::Sprite marco;
        marco.setTexture(*texturaMarco);
        sf::Vector2u size = texturaMarco->getSize();
        if (size.x > 0 && size.y > 0)
        {
            marco.setScale(76.0f / static_cast<float>(size.x), 76.0f / static_cast<float>(size.y));
        }
        marco.setPosition(x, y);
        window.draw(marco);
    }
    else
    {
        sf::RectangleShape marcoFallback(sf::Vector2f(76.0f, 76.0f));
        marcoFallback.setPosition(x, y);
        marcoFallback.setFillColor(sf::Color(30, 30, 40, 220));
        sf::Color colorFallback = sf::Color(80, 220, 80);
        if (personaje == 1)
            colorFallback = sf::Color(255, 90, 60);
        else if (personaje == 2)
            colorFallback = sf::Color(70, 170, 255);
        else if (personaje == 3)
            colorFallback = sf::Color(150, 80, 255);
        marcoFallback.setOutlineColor(colorFallback);
        marcoFallback.setOutlineThickness(3.0f);
        window.draw(marcoFallback);
    }

    if (fuente != nullptr)
    {
        sf::Text texto;
        texto.setFont(*fuente);
        texto.setCharacterSize(14);
        texto.setFillColor(sf::Color::White);
        texto.setOutlineColor(sf::Color::Black);
        texto.setOutlineThickness(2.0f);
        texto.setString(etiqueta);
        texto.setPosition(x + 5.0f, y + 4.0f);
        window.draw(texto);

    }

    dibujarCorazonesVidaHUD(window, knight.getVidas(), x + 13.0f, y + 94.0f);

    for (int i = 0; i < rondasGanadas; i++)
    {
        dibujarBombaDoradaHUD(window, x + 18.0f + i * 19.0f, y + 118.0f, 0.82f);
    }

    float barrasX = barrasALaIzquierda ? x - 136.0f : x + 86.0f;
    dibujarBarrasItemsHUD(window, knight, barrasX, y + 4.0f);
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

#include "GestorArcade.cpp"

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
        "assets/images/Victoria_verde.png",
        "assets/images/Victoria_rojo.png",
        "assets/images/Victoria_azul.png",
        "assets/images/Victoria_negro.png"
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

std::string obtenerRutaMarcoPersonaje(int personaje)
{
    const std::string rutas[4] = {
        "assets/images/marco_verde.png",
        "assets/images/marco_rojo.png",
        "assets/images/marco_azul.png",
        "assets/images/marco_negro.png"
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

std::vector<sf::Vector2i> generarPatronMuerteSubita()
{
    std::vector<sf::Vector2i> patron;
    int top = 1;
    int bottom = 11;
    int left = 1;
    int right = 13;

    while (top <= bottom && left <= right)
    {
        for (int columna = left; columna <= right; columna++)
            patron.push_back({top, columna});
        top++;

        for (int fila = top; fila <= bottom; fila++)
            patron.push_back({fila, right});
        right--;

        if (top <= bottom)
        {
            for (int columna = right; columna >= left; columna--)
                patron.push_back({bottom, columna});
            bottom--;
        }

        if (left <= right)
        {
            for (int fila = bottom; fila >= top; fila--)
                patron.push_back({fila, left});
            left++;
        }
    }

    return patron;
}

std::string formatearTiempo(float segundos)
{
    int total = static_cast<int>(std::ceil(std::max(0.0f, segundos)));
    int minutos = total / 60;
    int segundosRestantes = total % 60;

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(2) << minutos
           << ":" << std::setw(2) << segundosRestantes;
    return stream.str();
}

sf::Vector2i obtenerCeldaBody(b2BodyId bodyId)
{
    sf::Vector2f centro = obtenerCentroBody(bodyId);
    return sf::Vector2i(
        static_cast<int>(centro.y / MAP_CELL_SIZE),
        static_cast<int>(centro.x / MAP_CELL_SIZE)
    );
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
    MenuArcadePlayers menuArcadePlayers;
    menuArcadePlayers.init();
    SeleccionPersonaje pantallaSeleccion;
    pantallaSeleccion.init();
    Puerta puertaSalida;
    puertaSalida.init();
    GestorArcade gestorArcade;
    int modoSeleccionado = 0;
    int jugadoresArcade = 1;
    int personajeSeleccionadoP1 = 0;
    int personajeSeleccionadoP2 = 2;
    int ganadorVersus = 0;
    int personajeGanadorVersus = 0;
    int rondasGanadasP1 = 0;
    int rondasGanadasP2 = 0;
    bool reinicioRondaPendiente = false;
    int perdedorRondaPendiente = 0;
    float tiempoRonda = DURACION_RONDA_VERSUS;
    bool muerteSubita = false;
    float caidaTimer = INTERVALO_CAIDA_MUERTE_SUBITA;
    std::size_t indiceCaidaMuerteSubita = 0;
    std::vector<sf::Vector2i> patronMuerteSubita = generarPatronMuerteSubita();
    sf::Texture texturaGanadorVersus;
    sf::Sprite spriteGanadorVersus;
    bool texturaGanadorVersusCargada = false;
    sf::Texture texturaEmpateVersus;
    sf::Sprite spriteEmpateVersus;
    bool texturaEmpateVersusCargada = false;
    sf::Texture texturaVictoriaArcade;
    sf::Sprite spriteVictoriaArcade;
    bool texturaVictoriaArcadeCargada = texturaVictoriaArcade.loadFromFile("assets/images/CONGRATULATIONS.png");
    if (texturaVictoriaArcadeCargada)
    {
        spriteVictoriaArcade.setTexture(texturaVictoriaArcade, true);
        sf::Vector2u size = texturaVictoriaArcade.getSize();
        if (size.x > 0 && size.y > 0)
        {
            spriteVictoriaArcade.setScale(
                960.0f / static_cast<float>(size.x),
                832.0f / static_cast<float>(size.y)
            );
        }
    }

    std::array<sf::Texture, 4> texturasMarcosPersonajes;
    std::array<bool, 4> marcosPersonajesCargados = {false, false, false, false};
    for (int i = 0; i < 4; i++)
    {
        marcosPersonajesCargados[i] = texturasMarcosPersonajes[i].loadFromFile(obtenerRutaMarcoPersonaje(i));
        if (!marcosPersonajesCargados[i])
        {
            std::cout << "Aviso: no se pudo cargar " << obtenerRutaMarcoPersonaje(i) << std::endl;
        }
    }

    // SISTEMA DE AUDIO (FASE 1)
    sf::Music musicaFondo;
    // Cargar música de fondo (archivo ficticio - modificar cuando exista el archivo real)
    if (!musicaFondo.openFromFile("assets/audio/menu_medieval_bomber.wav"))
    {
        std::cout << "Aviso: No se pudo cargar assets/audio/menu_medieval_bomber.wav" << std::endl;
    }
    else
    {
        musicaFondo.setLoop(true);
        musicaFondo.setVolume(48.0f);
        musicaFondo.play();
    }

    // INTERFAZ GRÁFICA (FASE 5)
    bool musicaPartidaActiva = false;
    auto cambiarMusicaFondo = [&](bool usarPartida)
    {
        if (usarPartida == musicaPartidaActiva)
        {
            return;
        }

        const char* rutaMusica = usarPartida
            ? "assets/audio/partida_medieval_bomber.wav"
            : "assets/audio/menu_medieval_bomber.wav";

        musicaFondo.stop();
        if (!musicaFondo.openFromFile(rutaMusica))
        {
            std::cout << "Aviso: No se pudo cargar " << rutaMusica << std::endl;
            return;
        }

        musicaPartidaActiva = usarPartida;
        musicaFondo.setLoop(true);
        musicaFondo.setVolume(usarPartida ? 52.0f : 48.0f);
        musicaFondo.play();
    };

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

    sf::Text textoTiempoVersus;
    if (fuenteCargada)
    {
        textoTiempoVersus.setFont(fuente);
        textoTiempoVersus.setCharacterSize(32);
        textoTiempoVersus.setFillColor(sf::Color::White);
        textoTiempoVersus.setOutlineColor(sf::Color::Black);
        textoTiempoVersus.setOutlineThickness(3.0f);
        textoTiempoVersus.setString("03:00");
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
        textoVolverMenu.setOutlineThickness(3.0f);
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
        mapa.setTemaObjetos(-1);
        mapa.inicializarGrid();
        mapa.generarFisicas(physics);

        knight.reiniciar(96.0f, 96.0f);
        knight2.reiniciar(-500.0f, -500.0f);
        modoActual = ARCADE;
        nivelActual = 1;
        ganadorVersus = 0;
        personajeGanadorVersus = 0;
        rondasGanadasP1 = 0;
        rondasGanadasP2 = 0;
        reinicioRondaPendiente = false;
        perdedorRondaPendiente = 0;
        tiempoRonda = DURACION_RONDA_VERSUS;
        muerteSubita = false;
        caidaTimer = INTERVALO_CAIDA_MUERTE_SUBITA;
        indiceCaidaMuerteSubita = 0;
        corazonesSpawneados = 0;
        texturaGanadorVersusCargada = false;
        texturaEmpateVersusCargada = false;
        puertaSalida.ocultar();
        mapa.setTemaObjetos(-1);
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
            sf::Vector2u size = texturaGanadorVersus.getSize();
            if (size.x > 0 && size.y > 0)
            {
                spriteGanadorVersus.setOrigin(0.0f, 0.0f);
                spriteGanadorVersus.setPosition(0.0f, 0.0f);
                spriteGanadorVersus.setScale(
                    960.0f / static_cast<float>(size.x),
                    832.0f / static_cast<float>(size.y)
                );
            }
        }

        estadoActual = VICTORIA_VERSUS;
        std::cout << "PLAYER " << ganador << " GANA" << std::endl;
    };

    auto cargarEmpateVersus = [&]()
    {
        texturaEmpateVersusCargada = texturaEmpateVersus.loadFromFile("assets/images/empate.png");
        if (texturaEmpateVersusCargada)
        {
            spriteEmpateVersus.setTexture(texturaEmpateVersus, true);
            sf::FloatRect bounds = spriteEmpateVersus.getLocalBounds();
            spriteEmpateVersus.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
            spriteEmpateVersus.setPosition(480.0f, 416.0f);
        }

        estadoActual = EMPATE_VERSUS;
        std::cout << "EMPATE VERSUS" << std::endl;
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
        mapa.setTemaObjetos(-1);
        mapa.inicializarGrid();
        mapa.generarFisicas(physics);
        llenarItemsIniciales(mapa, listaItems, corazonesSpawneados);

        sf::Vector2f spawnP1 = obtenerSpawnVersus(personajeSeleccionadoP1);
        sf::Vector2f spawnP2 = obtenerSpawnVersus(personajeSeleccionadoP2);
        knight.reiniciarRonda(spawnP1.x, spawnP1.y);
        knight2.reiniciarRonda(spawnP2.x, spawnP2.y);
        reinicioRondaPendiente = false;
        perdedorRondaPendiente = 0;
        tiempoRonda = DURACION_RONDA_VERSUS;
        muerteSubita = false;
        caidaTimer = INTERVALO_CAIDA_MUERTE_SUBITA;
        indiceCaidaMuerteSubita = 0;
        estadoActual = JUGANDO;
        std::cout << "RONDA VERSUS REINICIADA" << std::endl;
    };

    auto resolverDanoVersus = [&](int jugadorPerdedor)
    {
        bool p1SinVidas = knight.getVidas() <= 0;
        bool p2SinVidas = knight2.getVidas() <= 0;

        if (p1SinVidas && p2SinVidas)
        {
            cargarEmpateVersus();
            return;
        }

        if (p1SinVidas)
        {
            rondasGanadasP2++;
            cargarVictoriaVersus(2);
            return;
        }

        if (p2SinVidas)
        {
            rondasGanadasP1++;
            cargarVictoriaVersus(1);
            return;
        }

        if (jugadorPerdedor == 3)
        {
            reiniciarRondaVersus();
            return;
        }

        if (jugadorPerdedor == 1)
        {
            rondasGanadasP2++;
            if (p1SinVidas)
            {
                cargarVictoriaVersus(2);
                return;
            }
        }
        else if (jugadorPerdedor == 2)
        {
            rondasGanadasP1++;
            if (p2SinVidas)
            {
                cargarVictoriaVersus(1);
                return;
            }
        }

        reiniciarRondaVersus();
    };

    auto colocarSiguienteBloqueMuerteSubita = [&]()
    {
        while (indiceCaidaMuerteSubita < patronMuerteSubita.size())
        {
            sf::Vector2i celda = patronMuerteSubita[indiceCaidaMuerteSubita++];
            int fila = celda.x;
            int columna = celda.y;

            if (mapa.obtenerTipoCelda(fila, columna) == Mapa::INDESTRUCTIBLE)
            {
                continue;
            }

            sf::Vector2i celdaP1 = obtenerCeldaBody(knight.getBodyId());
            sf::Vector2i celdaP2 = obtenerCeldaBody(knight2.getBodyId());
            bool aplastaP1 = celdaP1.x == fila && celdaP1.y == columna;
            bool aplastaP2 = celdaP2.x == fila && celdaP2.y == columna;

            if (aplastaP1)
            {
                knight.setVidas(knight.getVidas() - 1);
            }
            if (aplastaP2)
            {
                knight2.setVidas(knight2.getVidas() - 1);
            }

            for (auto& bomba : listaBombas)
            {
                if (bomba.isActiva() && bomba.estaEnCelda(fila, columna))
                {
                    bomba.forzarExplosion();
                }
            }

            auto item = listaItems.begin();
            while (item != listaItems.end())
            {
                if (item->estaEnCelda(fila, columna))
                {
                    item = listaItems.erase(item);
                }
                else
                {
                    ++item;
                }
            }

            mapa.colocarBloqueIndestructible(fila, columna, physics);

            if (estadoActual == JUGANDO && (aplastaP1 || aplastaP2))
            {
                reinicioRondaPendiente = true;
                perdedorRondaPendiente = (aplastaP1 && aplastaP2) ? 3 : (aplastaP1 ? 1 : 2);
            }

            break;
        }
    };

    auto estaEnGameplay = [&]()
    {
        return estadoActual == JUGANDO || estadoActual == JUGANDO_ARCADE;
    };

    auto segundoJugadorActivo = [&]()
    {
        return modoActual == MULTIJUGADOR ||
               (estadoActual == JUGANDO_ARCADE && gestorArcade.esCooperativo());
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
                        estadoActual = MENU_ARCADE_PLAYERS;
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

            if (estadoActual == MENU_ARCADE_PLAYERS)
            {
                menuArcadePlayers.handleInput(event);
                if (menuArcadePlayers.seleccionConfirmada())
                {
                    jugadoresArcade = menuArcadePlayers.getCantidadJugadores();
                    menuArcadePlayers.limpiarConfirmacion();
                    pantallaSeleccion.setModoMultijugador(jugadoresArcade == 2);
                    estadoActual = SELECCION_ARCADE;
                }

                continue;
            }

            if (estadoActual == SELECCION_PERSONAJE || estadoActual == SELECCION_ARCADE)
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

                    if (estadoActual == SELECCION_ARCADE)
                    {
                        modoActual = ARCADE;
                        gestorArcade.iniciar(jugadoresArcade);
                        temporizadoresRespawnItems.clear();
                        gestorArcade.cargarNivelActual(
                            mapa,
                            physics,
                            knight,
                            knight2,
                            listaEnemigos,
                            listaJefes,
                            listaBombas,
                            listaExplosiones,
                            listaItems,
                            puertaSalida,
                            corazonesSpawneados
                        );
                        nivelActual = gestorArcade.getNivelActual();
                        estadoActual = JUGANDO_ARCADE;
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
                        mapa.setTemaObjetos(-1);
                        mapa.inicializarGrid();
                        mapa.generarFisicas(physics);
                        llenarItemsIniciales(mapa, listaItems, corazonesSpawneados);

                        sf::Vector2f spawnP1 = obtenerSpawnVersus(personajeP1);
                        sf::Vector2f spawnP2 = obtenerSpawnVersus(personajeP2);
                        knight.reiniciar(spawnP1.x, spawnP1.y);
                        knight2.reiniciar(spawnP2.x, spawnP2.y);
                        tiempoRonda = DURACION_RONDA_VERSUS;
                        muerteSubita = false;
                        caidaTimer = INTERVALO_CAIDA_MUERTE_SUBITA;
                        indiceCaidaMuerteSubita = 0;
                        rondasGanadasP1 = 0;
                        rondasGanadasP2 = 0;
                        nivelActual = 1;
                        estadoActual = JUGANDO;
                    }
                }

                continue;
            }

            // Manejar input de Espacio para plantar bomba (FASE 2)
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Space && estaEnGameplay())
                {
                    knight.plantarBomba(mapa, listaBombas, physics, 1);
                }
                if (event.key.code == sf::Keyboard::Numpad0 && estaEnGameplay() && segundoJugadorActivo())
                {
                    knight2.plantarBomba(mapa, listaBombas, physics, 2);
                }
                if (event.key.code == sf::Keyboard::Return &&
                    (estadoActual == VICTORIA_VERSUS || estadoActual == EMPATE_VERSUS || estadoActual == VICTORIA_ARCADE))
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
                        mapa.setTemaObjetos(-1);
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
                        gestorArcade.iniciar(jugadoresArcade);
                        temporizadoresRespawnItems.clear();
                        gestorArcade.cargarNivelActual(
                            mapa,
                            physics,
                            knight,
                            knight2,
                            listaEnemigos,
                            listaJefes,
                            listaBombas,
                            listaExplosiones,
                            listaItems,
                            puertaSalida,
                            corazonesSpawneados
                        );
                        nivelActual = gestorArcade.getNivelActual();
                        estadoActual = JUGANDO_ARCADE;
                    }
                }
            }
        }

        cambiarMusicaFondo(
            estadoActual == JUGANDO ||
            estadoActual == JUGANDO_ARCADE ||
            estadoActual == TRANSICION_NIVEL
        );

        if (estadoActual == MENU_PRINCIPAL)
        {
            window.clear(sf::Color(24, 28, 38));
            menu.draw(window);
            window.display();
            continue;
        }

        if (estadoActual == MENU_ARCADE_PLAYERS)
        {
            window.clear(sf::Color(18, 20, 28));
            menuArcadePlayers.draw(window);
            window.display();
            continue;
        }

        if (estadoActual == SELECCION_PERSONAJE || estadoActual == SELECCION_ARCADE)
        {
            pantallaSeleccion.update();
            window.clear(sf::Color(22, 24, 34));
            pantallaSeleccion.draw(window);
            window.display();
            continue;
        }

        if (estadoActual == TRANSICION_NIVEL)
        {
            if (gestorArcade.consumirSolicitudAvance())
            {
                gestorArcade.cargarNivelActual(
                    mapa,
                    physics,
                    knight,
                    knight2,
                    listaEnemigos,
                    listaJefes,
                    listaBombas,
                    listaExplosiones,
                    listaItems,
                    puertaSalida,
                    corazonesSpawneados
                );
                nivelActual = gestorArcade.getNivelActual();
                temporizadoresRespawnItems.clear();
                estadoActual = JUGANDO_ARCADE;
            }
            else
            {
                estadoActual = VICTORIA_ARCADE;
            }

            continue;
        }

        if (estadoActual == VICTORIA_ARCADE)
        {
            window.clear(sf::Color(10, 10, 16));
            if (texturaVictoriaArcadeCargada)
            {
                window.draw(spriteVictoriaArcade);
            }

            if (fuenteCargada)
            {
                textoVictoria.setString("CONGRATULATIONS");
                sf::FloatRect victoriaBounds = textoVictoria.getLocalBounds();
                textoVictoria.setPosition(
                    480.0f - victoriaBounds.left - victoriaBounds.width / 2.0f,
                    260.0f
                );
                textoVolverMenu.setString("Presiona ENTER para volver al menu");
                sf::FloatRect volverBounds = textoVolverMenu.getLocalBounds();
                textoVolverMenu.setPosition(
                    480.0f - volverBounds.left - volverBounds.width / 2.0f,
                    792.0f
                );

                if (!texturaVictoriaArcadeCargada)
                {
                    window.draw(textoVictoria);
                }
                window.draw(textoVolverMenu);
            }

            window.display();
            continue;
        }

        // INPUT
        knight.handleInput();
        if (segundoJugadorActivo())
        {
            knight2.handleInput();
        }

        // PHYSICS STEP (Box2D v3.0)
        physics.step(timeStep);

        // SYNC GRAPHICS WITH PHYSICS
        knight.syncWithPhysics();
        if (segundoJugadorActivo())
        {
            knight2.syncWithPhysics();
        }

        // UPDATE SPRITES
        knight.update(timeStep);
        if (segundoJugadorActivo())
        {
            knight2.update(timeStep);
            intentarPatearBombas(knight, listaBombas);
            intentarPatearBombas(knight2, listaBombas);
        }

        // UPDATE BOMBAS (FASE 2) + EXPLOSIONES (FASE 3)
        std::vector<b2BodyId> jugadoresBodyId = {knight.getBodyId()};
        if (segundoJugadorActivo())
        {
            jugadoresBodyId.push_back(knight2.getBodyId());
        }

        bool revisarCadenaBombas = true;
        int guardiaCadenaBombas = 0;
        while (revisarCadenaBombas && guardiaCadenaBombas < 128)
        {
            revisarCadenaBombas = false;
            guardiaCadenaBombas++;

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
                if (forzarBombasTocadasPorExplosion(explosion, listaBombas))
                {
                    revisarCadenaBombas = true;
                }
                destruirItemsTocadosPorExplosion(explosion, listaItems);
                listaExplosiones.push_back(explosion);
            }
            bomba.actualizarMovimientoPateado(mapa);
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

        mapa.update(timeStep);

        if (estadoActual == JUGANDO && modoActual == MULTIJUGADOR && !reinicioRondaPendiente)
        {
            if (!muerteSubita)
            {
                tiempoRonda -= timeStep;
                if (tiempoRonda <= 0.0f)
                {
                    tiempoRonda = 0.0f;
                    muerteSubita = true;
                    caidaTimer = 0.0f;
                }
            }
            else
            {
                caidaTimer -= timeStep;
                if (caidaTimer <= 0.0f)
                {
                    caidaTimer = INTERVALO_CAIDA_MUERTE_SUBITA;
                    colocarSiguienteBloqueMuerteSubita();
                }
            }
        }

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
            b2Vec2 posBoss = b2Body_GetPosition(boss.getBodyId());
            float pixelXBoss = posBoss.x * PIXELS_PER_METER;
            float pixelYBoss = posBoss.y * PIXELS_PER_METER;

            b2Vec2 posKnightBoss = b2Body_GetPosition(knight.getBodyId());
            float pixelXKnight = posKnightBoss.x * PIXELS_PER_METER;
            float pixelYKnight = posKnightBoss.y * PIXELS_PER_METER;
            float distancia = std::sqrt((pixelXKnight - pixelXBoss) * (pixelXKnight - pixelXBoss) +
                                        (pixelYKnight - pixelYBoss) * (pixelYKnight - pixelYBoss));

            if (distancia < 64.0f)
            {
                if (knight.recibirDano(physics, modoActual == MULTIJUGADOR) && modoActual == MULTIJUGADOR)
                {
                    danoVersusP1 = true;
                }
            }

            if (segundoJugadorActivo())
            {
                b2Vec2 posKnight2Boss = b2Body_GetPosition(knight2.getBodyId());
                float pixelXKnight2 = posKnight2Boss.x * PIXELS_PER_METER;
                float pixelYKnight2 = posKnight2Boss.y * PIXELS_PER_METER;
                float distanciaP2 = std::sqrt((pixelXKnight2 - pixelXBoss) * (pixelXKnight2 - pixelXBoss) +
                                              (pixelYKnight2 - pixelYBoss) * (pixelYKnight2 - pixelYBoss));

                if (distanciaP2 < 64.0f)
                {
                    if (knight2.recibirDano(physics, modoActual == MULTIJUGADOR) && modoActual == MULTIJUGADOR)
                    {
                        danoVersusP2 = true;
                    }
                }
            }
        }

        if (estadoActual == JUGANDO_ARCADE)
        {
            gestorArcade.update(mapa, physics, listaEnemigos, listaJefes, puertaSalida);
            nivelActual = gestorArcade.getNivelActual();

            if (gestorArcade.consumirVictoriaArcade())
            {
                estadoActual = VICTORIA_ARCADE;
            }
            else if (knight.getVidas() <= 0 && (!gestorArcade.esCooperativo() || knight2.getVidas() <= 0))
            {
                estadoActual = GAME_OVER;
                std::cout << "GAME OVER ARCADE - Presiona ENTER para reiniciar" << std::endl;
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
        if (segundoJugadorActivo())
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

        if (estadoActual == JUGANDO_ARCADE && puertaSalida.estaAbierta())
        {
            bool p1EnPuerta = puertaSalida.jugadorEstaEncima(pixelXKnight, pixelYKnight);
            bool p2EnPuerta = false;
            if (segundoJugadorActivo())
            {
                b2Vec2 posKnight2Puerta = b2Body_GetPosition(knight2.getBodyId());
                p2EnPuerta = puertaSalida.jugadorEstaEncima(
                    posKnight2Puerta.x * PIXELS_PER_METER,
                    posKnight2Puerta.y * PIXELS_PER_METER
                );
            }

            if (p1EnPuerta || p2EnPuerta)
            {
                gestorArcade.solicitarAvanceNivel();
                estadoActual = TRANSICION_NIVEL;
            }
        }

        // Recopilar items
        for (auto& item : listaItems)
        {
            if (item.isActivo() && item.estaVisible() && item.getFila() == filaKnight && item.getColumna() == colKnight)
            {
                knight.recolectarItem(item.getTipo(), modoActual == MULTIJUGADOR || estadoActual == JUGANDO_ARCADE);
                item.desactivar();
                if (modoActual != MULTIJUGADOR)
                {
                    temporizadoresRespawnItems.push_back(TIEMPO_RESPAWN_ITEM);
                }
            }
            else if (segundoJugadorActivo() &&
                     item.isActivo() &&
                     item.estaVisible() &&
                     item.getFila() == filaKnight2 &&
                     item.getColumna() == colKnight2)
            {
                knight2.recolectarItem(item.getTipo(), modoActual == MULTIJUGADOR || estadoActual == JUGANDO_ARCADE);
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
                if (segundoJugadorActivo())
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
            perdedorRondaPendiente = (danoVersusP1 && danoVersusP2) ? 3 : (danoVersusP1 ? 1 : 2);
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
            else if (estadoActual == JUGANDO_ARCADE)
            {
                hudText = "ARCADE NIVEL " + std::to_string(gestorArcade.getNivelActual()) +
                          " | " + gestorArcade.getNombreNivelActual() +
                          " | P1 VIDAS: " + std::to_string(knight.getVidas());
                if (gestorArcade.esCooperativo())
                {
                    hudText += " | P2 VIDAS: " + std::to_string(knight2.getVidas());
                }
                hudText += " | ENEMIGOS: " + std::to_string(listaEnemigos.size());
            }
            else
            {
                hudText = "NIVEL: " + std::to_string(nivelActual) + 
                          " | VIDAS: " + std::to_string(knight.getVidas()) + 
                          " | ENEMIGOS: " + std::to_string(listaEnemigos.size());
            }
            textoHUD.setString(hudText);

            if (modoActual == MULTIJUGADOR)
            {
                textoTiempoVersus.setString(formatearTiempo(tiempoRonda));
                textoTiempoVersus.setFillColor(muerteSubita ? sf::Color::Red : sf::Color::White);
                sf::FloatRect boundsTiempo = textoTiempoVersus.getLocalBounds();
                textoTiempoVersus.setPosition(
                    480.0f - boundsTiempo.width / 2.0f,
                    8.0f
                );
            }
        }

        // RENDER
        window.clear(sf::Color(35, 35, 45));
        mapa.draw(window);
        if (estadoActual == JUGANDO_ARCADE || estadoActual == TRANSICION_NIVEL)
        {
            puertaSalida.draw(window);
        }
        
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
        if (segundoJugadorActivo())
        {
            knight2.draw(window);
        }

        // RENDER HUD (FASE 5)
        if (fuenteCargada)
        {
            sf::Texture* marcoP1 = marcosPersonajesCargados[personajeSeleccionadoP1]
                ? &texturasMarcosPersonajes[personajeSeleccionadoP1]
                : nullptr;
            dibujarPanelJugadorHUD(
                window,
                knight,
                personajeSeleccionadoP1,
                marcoP1,
                (modoActual == MULTIJUGADOR) ? rondasGanadasP1 : 0,
                "P1",
                &fuente,
                10.0f,
                8.0f
            );

            if (segundoJugadorActivo())
            {
                sf::Texture* marcoP2 = marcosPersonajesCargados[personajeSeleccionadoP2]
                    ? &texturasMarcosPersonajes[personajeSeleccionadoP2]
                    : nullptr;
                dibujarPanelJugadorHUD(
                    window,
                    knight2,
                    personajeSeleccionadoP2,
                    marcoP2,
                    (modoActual == MULTIJUGADOR) ? rondasGanadasP2 : 0,
                    "P2",
                    &fuente,
                    874.0f,
                    8.0f,
                    true
                );
            }

            if (modoActual == MULTIJUGADOR)
            {
                window.draw(textoTiempoVersus);
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
            if (texturaGanadorVersusCargada)
            {
                window.draw(spriteGanadorVersus);
            }

            if (fuenteCargada)
            {
                textoVictoriaVersus.setString(std::string("PLAYER ") + std::to_string(ganadorVersus) + " GANA!");
                sf::FloatRect victoriaBounds = textoVictoriaVersus.getLocalBounds();
                textoVictoriaVersus.setPosition(
                    480.0f - victoriaBounds.left - victoriaBounds.width / 2.0f,
                    692.0f
                );

                sf::FloatRect volverBounds = textoVolverMenu.getLocalBounds();
                textoVolverMenu.setPosition(
                    480.0f - volverBounds.left - volverBounds.width / 2.0f,
                    792.0f
                );

                window.draw(textoVictoriaVersus);
                window.draw(textoVolverMenu);
            }
        }

        if (estadoActual == EMPATE_VERSUS)
        {
            sf::RectangleShape overlay(sf::Vector2f(960.0f, 832.0f));
            overlay.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(overlay);

            if (texturaEmpateVersusCargada)
            {
                window.draw(spriteEmpateVersus);
            }

            if (fuenteCargada)
            {
                if (!texturaEmpateVersusCargada)
                {
                    textoVictoriaVersus.setString("EMPATE");
                    textoVictoriaVersus.setPosition(
                        480.0f - textoVictoriaVersus.getLocalBounds().width / 2.0f,
                        260.0f
                    );
                    window.draw(textoVictoriaVersus);
                }
                window.draw(textoVolverMenu);
            }
        }

        window.display();
    }

    return 0;
}
