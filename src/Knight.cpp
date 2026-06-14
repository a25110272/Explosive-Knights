#include "Knight.hpp"

#include <iostream>

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

        frameWidth = 230;
        frameHeight = 235;
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
        direccion = dir;
        currentFrame = 0;
        caminando = false;
        caminandoAnterior = false;
        clock.restart();

        int fila = obtenerFilaDireccion();
        sprite.setTextureRect(sf::IntRect(
            0,
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
            if (!estabaCaminando)
            {
                currentFrame = 1;
                clock.restart();
            }

            if (clock.getElapsedTime().asSeconds() >= frameTime)
            {
                currentFrame++;

                if (currentFrame >= numFrames)
                {
                    currentFrame = 1;
                }

                clock.restart();
            }
        }
        else
        {
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

    float frameTime = 0.10f;

    int obtenerFilaDireccion()
    {
        if (direccion == ABAJO)
        {
            return 0;
        }
        else if (direccion == ARRIBA)
        {
            return 1;
        }
        else if (direccion == DERECHA)
        {
            return 3;
        }
        else
        {
            return 2;
        }
    }

    void centrarSprite()
    {
        sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
    }
};

Knight::Knight(sf::Vector2f position, PhysicsSpace& physics)
    : personaje(std::make_unique<Personaje>(position)),
      physicsSpace(physics),
      maxBombas(1),
      rangoFuego(1),
      speed(10.0f),
      speedOriginal(10.0f),
      vidas(3)
{
    bodyId = physicsSpace.createDynamicBody(
        position.x,
        position.y,
        KNIGHT_HITBOX_SIZE,
        KNIGHT_HITBOX_SIZE
    );

    posicionOriginal = position;
    direccionActual = ABAJO;
}

Knight::~Knight() = default;

void Knight::handleInput()
{
    b2Vec2 velocity = {0.0f, 0.0f};
    Direccion dirActual = static_cast<Direccion>(direccionActual);
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

    if (seMovio)
    {
        personaje->move(0, 0, dirActual);
        direccionActual = dirActual;
    }
    else
    {
        personaje->detenerAnimacion(static_cast<Direccion>(direccionActual));
    }

    if (b2Body_IsValid(bodyId))
    {
        b2Body_SetLinearVelocity(bodyId, velocity);
    }
}

void Knight::syncWithPhysics()
{
    if (b2Body_IsValid(bodyId))
    {
        b2Vec2 pos = b2Body_GetPosition(bodyId);
        float pixelX = pos.x * PIXELS_PER_METER;
        float pixelY = pos.y * PIXELS_PER_METER;

        personaje->setPhysicsPosition(pixelX, pixelY);
    }
}

void Knight::update()
{
    personaje->update();
}

void Knight::draw(sf::RenderWindow& window)
{
    personaje->draw(window);
}

void Knight::plantarBomba(Mapa& mapa, std::vector<Bomba>& bombas, PhysicsSpace& physics)
{
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
        return;
    }

    b2Vec2 posKnight = b2Body_GetPosition(bodyId);
    float pixelX = posKnight.x * PIXELS_PER_METER;
    float pixelY = posKnight.y * PIXELS_PER_METER;

    int columna = static_cast<int>(pixelX / 64.0f);
    int fila = static_cast<int>(pixelY / 64.0f);

    if (fila < 0 || fila >= 13 || columna < 0 || columna >= 15)
    {
        return;
    }

    if (mapa.obtenerTipoCelda(fila, columna) != Mapa::VACIO)
    {
        return;
    }

    Bomba nuevaBomba(fila, columna, physics);
    bombas.push_back(nuevaBomba);
}

void Knight::recolectarItem(int tipo)
{
    if (tipo == 0)
    {
        speed = speedOriginal * 1.5f;
    }
    else if (tipo == 1)
    {
        rangoFuego++;
    }
    else if (tipo == 2)
    {
        maxBombas++;
    }
}

void Knight::morir(PhysicsSpace& physics)
{
    (void)physics;
    vidas--;

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

void Knight::cambiarSprite(const std::string& ruta)
{
    (void)ruta;
}

void Knight::configurarBomba(const std::string& ruta, sf::Color color)
{
    (void)ruta;
    (void)color;
}

void Knight::reiniciar(float x, float y)
{
    if (b2Body_IsValid(bodyId))
    {
        b2Transform transform = {{x / PIXELS_PER_METER, y / PIXELS_PER_METER}, b2Rot_identity};
        b2Body_SetTransform(bodyId, transform.p, transform.q);
        b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
    }

    personaje->setPhysicsPosition(x, y);
    maxBombas = 1;
    rangoFuego = 1;
    speed = speedOriginal;
    vidas = 3;
}

void Knight::moverA(float x, float y)
{
    if (b2Body_IsValid(bodyId))
    {
        b2Transform transform = {{x / PIXELS_PER_METER, y / PIXELS_PER_METER}, b2Rot_identity};
        b2Body_SetTransform(bodyId, transform.p, transform.q);
        b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
    }

    personaje->setPhysicsPosition(x, y);
}

b2BodyId Knight::getBodyId()
{
    return bodyId;
}

int Knight::getRangoFuego() const
{
    return rangoFuego;
}

int Knight::getMaxBombas() const
{
    return maxBombas;
}

int Knight::getVidas() const
{
    return vidas;
}
