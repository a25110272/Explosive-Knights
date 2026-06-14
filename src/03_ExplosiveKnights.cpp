#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <box2d/box2d.h>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "Mapa.hpp"

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
    JUGANDO,
    GAME_OVER,
    VICTORIA
};

// ============== PHYSICS ENGINE (Box2D v3.0) ==============
const float PIXELS_PER_METER = 30.0f;
const float MAP_CELL_SIZE = 64.0f;
const float WALL_HITBOX_SIZE = 56.0f;
const float KNIGHT_HITBOX_SIZE = 40.0f;
const float KNIGHT_COLLISION_RADIUS = 31.0f;
const float KNIGHT_DAMAGE_HITBOX_SIZE = 32.0f;
const float FIRE_DAMAGE_MARGIN = 4.0f;
const float ENEMY_HITBOX_SIZE = 36.0f;
const uint32_t COLLISION_DEFAULT = 0x0001;
const uint32_t COLLISION_PLAYER = 0x0002;
const uint32_t COLLISION_BOMB = 0x0004;
const uint32_t COLLISION_ALL = 0xFFFFFFFF;

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
    Personaje(sf::Vector2f position)
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
        float escala = MAP_CELL_SIZE / static_cast<float>(frameWidth);
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

#include "Mapa.cpp"

// ============== EXPLOSIÓN (FASE 3: Explosiones y Destrucción) ==============
class Explosion
{
public:
    Explosion(int centroFila, int centroCol, Mapa& mapa, int rango = 2, std::vector<PowerUp>* pItems = nullptr)
        : activa(true), pListaItems(pItems)
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
                sf::RectangleShape fuego(sf::Vector2f(64.0f, 64.0f));
                fuego.setFillColor(sf::Color(255, 165, 0));  // Naranja
                fuego.setPosition(celda.y * 64.0f, celda.x * 64.0f);
                window.draw(fuego);
            }
        }
    }

    bool isActiva() const { return activa; }

    const std::vector<sf::Vector2i>& getCeldasAfectadas() const { return celdasAfectadas; }

private:
    std::vector<sf::Vector2i> celdasAfectadas;
    sf::Clock temporizador;
    bool activa;
    std::vector<PowerUp>* pListaItems;
};

// ============== FIN EXPLOSIÓN ==============

// ============== BOMBA (FASE 2: Sistema de Bombas) ==============
class Bomba
{
public:
    Bomba(int fila, int columna, PhysicsSpace& physics)
        : fila(fila), columna(columna), activa(true), jugadorEncima(true), solida(false),
          bodyId(b2_nullBodyId), shapeId(b2_nullShapeId)
    {
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
            shapeDef.density = 0.0f;
            shapeDef.filter.categoryBits = COLLISION_BOMB;
            shapeDef.filter.maskBits = COLLISION_ALL & ~COLLISION_PLAYER;
            shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
        }

        // Iniciar temporizador
        temporizador.restart();
    }

    bool update(PhysicsSpace& physics, b2BodyId jugadorBodyId)
    {
        // Verificar si superó los 3 segundos
        (void)physics;

        if (activa && jugadorEncima && !solida && b2Body_IsValid(jugadorBodyId))
        {
            b2Vec2 posJugador = b2Body_GetPosition(jugadorBodyId);
            float jugadorX = posJugador.x * PIXELS_PER_METER;
            float jugadorY = posJugador.y * PIXELS_PER_METER;

            float bombaX = columna * 64.0f + 32.0f;
            float bombaY = fila * 64.0f + 32.0f;

            bool aunSeTraslapan =
                std::abs(jugadorX - bombaX) < 48.0f &&
                std::abs(jugadorY - bombaY) < 48.0f;

            if (!aunSeTraslapan)
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
            sf::CircleShape bomba(24.0f);
            bomba.setFillColor(sf::Color::Black);
            bomba.setPosition(posX - 24.0f, posY - 24.0f);  // Centrar
            window.draw(bomba);
        }
    }

    bool isActiva() const { return activa; }

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

private:
    int fila;
    int columna;
    bool activa;
    bool jugadorEncima;
    bool solida;
    b2BodyId bodyId;
    b2ShapeId shapeId;
    sf::Clock temporizador;
};

// ============== FIN BOMBA ==============

// ============== KNIGHT (JUGADOR) - Box2D v3.0 ==============
class Knight
{
public:
    Knight(sf::Vector2f position, PhysicsSpace& physics)
        : personaje(position), physicsSpace(physics), 
          maxBombas(1), rangoFuego(1), speed(6.5f),
          speedOriginal(6.5f), vidas(3), tiempoInvulnerable(0.0f)
    {
        // Crear cuerpo dinámico en Box2D v3.0
        bodyId = physicsSpace.createDynamicCircleBody(
            position.x, 
            position.y, 
            KNIGHT_COLLISION_RADIUS,
            COLLISION_PLAYER,
            COLLISION_ALL
        );
        
        posicionOriginal = position;
    }

    void handleInput()
    {
        // Usar la velocidad del Knight (modificable por power-ups)
        
        b2Vec2 velocity = {0.0f, 0.0f};
        Direccion dirActual = direccionActual;
        bool seMovio = false;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            velocity.y = -speed;
            dirActual = ARRIBA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            velocity.y = speed;
            dirActual = ABAJO;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            velocity.x = speed;
            dirActual = DERECHA;
            seMovio = true;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
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

        personaje.update();
    }

    void draw(sf::RenderWindow& window)
    {
        personaje.draw(window);
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
        Bomba nuevaBomba(fila, columna, physics);
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
        recibirDano(physics);
    }

    void recibirDano(PhysicsSpace& physics)
    {
        (void)physics;
        if (vidas <= 0 || tiempoInvulnerable > 0.0f)
        {
            return;
        }

        vidas--;
        tiempoInvulnerable = 2.0f;

        if (vidas > 0)
        {
            float posX = 1.0f * 64.0f + 32.0f;
            float posY = 1.0f * 64.0f + 32.0f;

            if (b2Body_IsValid(bodyId))
            {
                b2Transform transform = {{posX / PIXELS_PER_METER, posY / PIXELS_PER_METER}, b2Rot_identity};
                b2Body_SetTransform(bodyId, transform.p, transform.q);
                b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
            }

            maxBombas = 1;
            rangoFuego = 1;
            speed = speedOriginal;
        }
    }

    void reiniciar(float x, float y)
    {
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
    float tiempoInvulnerable;
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
void cargarNivel(int nivel, Mapa& mapa, Knight& knight, std::vector<Enemigo>& enemigos,
                 std::vector<Bomba>& bombas, std::vector<Explosion>& explosiones,
                 std::vector<PowerUp>& items, std::vector<Boss>& jefes, PhysicsSpace& physics)
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
        jefes.emplace_back(bosX, bosY, physics, "assets/images/negro_spritesheet.png");
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
        enemigos.emplace_back(spawnPoints[i].first, spawnPoints[i].second, physics, "assets/images/rojo_spritesheet.png");
    }

    std::cout << "NIVEL " << nivel << " INICIADO - " << cantidadEnemigos << " ENEMIGOS" << std::endl;
}

// ============== FIN FUNCIÓN CARGAR NIVEL ==============

int main()
{
    sf::RenderWindow window(sf::VideoMode(960, 832), "Explosive-Knights - FASE 5");
    window.setFramerateLimit(60);

    // Máquina de estados y nivel (FASE 5)
    EstadoJuego estadoActual = JUGANDO;
    int nivelActual = 1;

    // Instanciar Physics Engine
    PhysicsSpace physics;

    // Instanciar Mapa
    Mapa mapa;
    // Instanciar Knight (jugador)
    Knight knight(sf::Vector2f(96.0f, 96.0f), physics);

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
    
    // Cargar primer nivel
    cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics);

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
                if (event.key.code == sf::Keyboard::Space && estadoActual == JUGANDO)
                {
                    knight.plantarBomba(mapa, listaBombas, physics);
                }
                // Reiniciar con Enter si es GAME_OVER o VICTORIA (FASE 5)
                if (event.key.code == sf::Keyboard::Return && (estadoActual == GAME_OVER || estadoActual == VICTORIA))
                {
                    estadoActual = JUGANDO;
                    nivelActual = 1;
                    knight.reiniciar(96.0f, 96.0f);
                    cargarNivel(nivelActual, mapa, knight, listaEnemigos, listaBombas, listaExplosiones, listaItems, listaJefes, physics);
                }
            }
        }

        // INPUT
        knight.handleInput();

        // PHYSICS STEP (Box2D v3.0)
        physics.step(timeStep);

        // SYNC GRAPHICS WITH PHYSICS
        knight.syncWithPhysics();

        // UPDATE SPRITES
        knight.update(timeStep);

        // UPDATE BOMBAS (FASE 2) + EXPLOSIONES (FASE 3)
        for (auto& bomba : listaBombas)
        {
            if (bomba.update(physics, knight.getBodyId()))  // Si retorna true = explotó
            {
                // Crear explosión con rango del Knight
                Explosion explosion(bomba.getFila(), bomba.getColumna(), mapa, knight.getRangoFuego(), &listaItems);
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

        mapa.update(timeStep);

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
                knight.recibirDano(physics);
            }
        }

        // LÓGICA DE VICTORIA/DERROTA (FASE 5 y 6)
        if (estadoActual == JUGANDO)
        {
            // Verificar si el jugador murió
            if (knight.getVidas() <= 0)
            {
                estadoActual = GAME_OVER;
                std::cout << "GAME OVER - Presiona ENTER para reiniciar" << std::endl;
            }
            // FASE 6: Si estamos en nivel 3, verificar si el Boss fue derrotado
            else if (nivelActual == 3 && listaJefes.empty())
            {
                estadoActual = VICTORIA;
                std::cout << "¡VICTORIA! ¡Derrotaste al Jefe Final! - Presiona ENTER para reiniciar" << std::endl;
            }
            // Verificar si ganó el nivel (todos los enemigos muertos) - Niveles 1 y 2
            else if (listaEnemigos.empty() && nivelActual < 3)
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
        sf::FloatRect knightDamageRect(
            pixelXKnight - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
            pixelYKnight - KNIGHT_DAMAGE_HITBOX_SIZE / 2.0f,
            KNIGHT_DAMAGE_HITBOX_SIZE,
            KNIGHT_DAMAGE_HITBOX_SIZE
        );

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
                    sf::FloatRect fireDamageRect(
                        celda.y * 64.0f + FIRE_DAMAGE_MARGIN,
                        celda.x * 64.0f + FIRE_DAMAGE_MARGIN,
                        64.0f - FIRE_DAMAGE_MARGIN * 2.0f,
                        64.0f - FIRE_DAMAGE_MARGIN * 2.0f
                    );

                    if (fireDamageRect.intersects(knightDamageRect))
                    {
                        knight.recibirDano(physics);
                        break;
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
                    knight.recibirDano(physics);
                }
            }
        }

        // ACTUALIZAR HUD (FASE 5)
        if (fuenteCargada)
        {
            std::string hudText = "NIVEL: " + std::to_string(nivelActual) + 
                                  " | VIDAS: " + std::to_string(knight.getVidas()) + 
                                  " | ENEMIGOS: " + std::to_string(listaEnemigos.size());
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
