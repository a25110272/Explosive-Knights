#include <SFML/Graphics.hpp>
#include <iostream>

enum Direccion
{
    ABAJO,
    ARRIBA,
    DERECHA,
    IZQUIERDA
};

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

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Explosive-Knights");
    window.setFramerateLimit(60);

    Personaje caballeroVerde(sf::Vector2f(400, 300));

    float velocidad = 3.0f;

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            caballeroVerde.move(0, -velocidad, ARRIBA);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            caballeroVerde.move(0, velocidad, ABAJO);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            caballeroVerde.move(velocidad, 0, DERECHA);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            caballeroVerde.move(-velocidad, 0, IZQUIERDA);
        }

        caballeroVerde.update();

        window.clear(sf::Color(35, 35, 45));

        caballeroVerde.draw(window);

        window.display();
    }

    return 0;
}