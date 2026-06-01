#include <SFML/Graphics.hpp>
#include <iostream>
#include <box2d/box2d.h>

enum Direccion
{
    ABAJO,
    ARRIBA,
    DERECHA,
    IZQUIERDA
};

// ============== PHYSICS ENGINE (Box2D v3.0) ==============
const float PIXELS_PER_METER = 30.0f;

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
        shapeDef.friction = 0.3f;

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
        sprite.setScale(1.0f, 1.0f);
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

// ============== KNIGHT (JUGADOR) - Box2D v3.0 ==============
class Knight
{
public:
    Knight(sf::Vector2f position, PhysicsSpace& physics)
        : personaje(position), physicsSpace(physics)
    {
        // Crear cuerpo dinámico en Box2D v3.0
        bodyId = physicsSpace.createDynamicBody(
            position.x, 
            position.y, 
            100.0f,  // ancho del hitbox
            120.0f   // alto del hitbox
        );
    }

    void handleInput()
    {
        // Velocidad: 10.0f m/s = 300 píxeles/seg a 60 FPS = 5 píxeles/frame
        const float speed = 10.0f;
        
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

    void update()
    {
        personaje.update();
    }

    void draw(sf::RenderWindow& window)
    {
        personaje.draw(window);
    }

    b2BodyId getBodyId() { return bodyId; }

private:
    Personaje personaje;
    b2BodyId bodyId;
    PhysicsSpace& physicsSpace;
    Direccion direccionActual = ABAJO;
};

// ============== FIN KNIGHT - Box2D v3.0 ==============

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Explosive-Knights");
    window.setFramerateLimit(60);

    // Instanciar Physics Engine
    PhysicsSpace physics;

    // Instanciar Knight (jugador)
    Knight knight(sf::Vector2f(400, 300), physics);

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
        }

        // INPUT
        knight.handleInput();

        // PHYSICS STEP (Box2D v3.0)
        physics.step(timeStep);

        // SYNC GRAPHICS WITH PHYSICS
        knight.syncWithPhysics();

        // UPDATE SPRITES
        knight.update();

        // RENDER
        window.clear(sf::Color(35, 35, 45));
        knight.draw(window);
        window.display();
    }

    return 0;
}