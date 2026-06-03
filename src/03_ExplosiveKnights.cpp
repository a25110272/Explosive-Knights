#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <box2d/box2d.h>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>

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
        sprite.setScale(0.25f, 0.25f);
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

// ============== MAPA (FASE 1: Escenario) ==============
class Mapa
{
public:
    // Tipos de celda
    static const int VACIO = 0;
    static const int INDESTRUCTIBLE = 1;
    static const int DESTRUCTIBLE = 2;

    Mapa()
    {
        srand(static_cast<unsigned>(time(0)));
        inicializarGrid();
    }

    void inicializarGrid()
    {
        // Inicializar matriz
        for (int i = 0; i < 13; i++)
        {
            for (int j = 0; j < 15; j++)
            {
                // Bordes del mapa
                if (i == 0 || i == 12 || j == 0 || j == 14)
                {
                    grid[i][j] = INDESTRUCTIBLE;
                }
                // Pilares fijos (filas pares Y columnas pares)
                else if (i % 2 == 0 && j % 2 == 0)
                {
                    grid[i][j] = INDESTRUCTIBLE;
                }
                // Celdas de inicio del jugador: siempre VACIO
                else if ((i == 1 && j == 1) || (i == 1 && j == 2) || (i == 2 && j == 1))
                {
                    grid[i][j] = VACIO;
                }
                // Resto: 40% probabilidad de DESTRUCTIBLE
                else
                {
                    int aleatorio = rand() % 100;
                    if (aleatorio < 40)
                    {
                        grid[i][j] = DESTRUCTIBLE;
                    }
                    else
                    {
                        grid[i][j] = VACIO;
                    }
                }
            }
        }
    }

    void generarFisicas(PhysicsSpace& physics)
    {
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
                    b2BodyId bodyId = physics.createStaticBody(posX, posY, 64.0f, 64.0f);
                    
                    // Guardar el ID en el map (fila, columna) -> b2BodyId
                    bodiesMap[{i, j}] = bodyId;
                }
            }
        }
    }

    void draw(sf::RenderWindow& window)
    {
        for (int i = 0; i < 13; i++)
        {
            for (int j = 0; j < 15; j++)
            {
                if (grid[i][j] != VACIO)
                {
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

    void destruirBloque(int fila, int columna)
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

private:
    int grid[13][15];
    std::map<std::pair<int, int>, b2BodyId> bodiesMap;  // Mapeo (fila, col) -> b2BodyId
};

// ============== FIN MAPA ==============

// ============== EXPLOSIÓN (FASE 3: Explosiones y Destrucción) ==============
class Explosion
{
public:
    Explosion(int centroFila, int centroCol, Mapa& mapa, int rango = 2)
        : activa(true)
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
                mapa.destruirBloque(fila, col);
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
                mapa.destruirBloque(fila, col);
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
                mapa.destruirBloque(fila, col);
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
                mapa.destruirBloque(fila, col);
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

private:
    std::vector<sf::Vector2i> celdasAfectadas;
    sf::Clock temporizador;
    bool activa;
};

// ============== FIN EXPLOSIÓN ==============

// ============== BOMBA (FASE 2: Sistema de Bombas) ==============
class Bomba
{
public:
    Bomba(int fila, int columna, PhysicsSpace& physics)
        : fila(fila), columna(columna), activa(true)
    {
        // Calcular posición en píxeles: centro de la celda
        float posX = columna * 64.0f + 32.0f;
        float posY = fila * 64.0f + 32.0f;

        // Crear cuerpo estático en Box2D v3.0 (48x48 para la bomba)
        bodyId = physics.createStaticBody(posX, posY, 48.0f, 48.0f);

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
            sf::CircleShape bomba(24.0f);
            bomba.setFillColor(sf::Color::Black);
            bomba.setPosition(posX - 24.0f, posY - 24.0f);  // Centrar
            window.draw(bomba);
        }
    }

    bool isActiva() const { return activa; }

    int getFila() const { return fila; }
    int getColumna() const { return columna; }

private:
    int fila;
    int columna;
    bool activa;
    b2BodyId bodyId;
    sf::Clock temporizador;
};

// ============== FIN BOMBA ==============

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
            48.0f,   // ancho del hitbox
            48.0f    // alto del hitbox
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

    void plantarBomba(Mapa& mapa, std::vector<Bomba>& bombas, PhysicsSpace& physics)
    {
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
    sf::RenderWindow window(sf::VideoMode(960, 832), "Explosive-Knights - FASE 3");
    window.setFramerateLimit(60);

    // Instanciar Physics Engine
    PhysicsSpace physics;

    // Instanciar Mapa
    Mapa mapa;
    
    // Generar físicas del mapa
    mapa.generarFisicas(physics);

    // Instanciar Knight (jugador)
    Knight knight(sf::Vector2f(96.0f, 96.0f), physics);

    // Vector de bombas (FASE 2)
    std::vector<Bomba> listaBombas;

    // Vector de explosiones (FASE 3)
    std::vector<Explosion> listaExplosiones;

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
                if (event.key.code == sf::Keyboard::Space)
                {
                    knight.plantarBomba(mapa, listaBombas, physics);
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
        knight.update();

        // UPDATE BOMBAS (FASE 2) + EXPLOSIONES (FASE 3)
        for (auto& bomba : listaBombas)
        {
            if (bomba.update(physics))  // Si retorna true = explotó
            {
                // Crear explosión con rango 2 en la posición de la bomba
                Explosion explosion(bomba.getFila(), bomba.getColumna(), mapa, 2);
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

        // RENDER
        window.clear(sf::Color(35, 35, 45));
        mapa.draw(window);
        
        // RENDER BOMBAS (FASE 2)
        for (auto& bomba : listaBombas)
        {
            bomba.draw(window);
        }
        
        knight.draw(window);
        window.display();
    }

    return 0;
}