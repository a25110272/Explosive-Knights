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
        if (!texturaFrente.loadFromFile("assets/images/rojo_frente.png"))
        {
            std::cout << "Error al cargar rojo_frente.png\n";
        }

        if (!texturaEspalda.loadFromFile("assets/images/rojo_espalda.png"))
        {
            std::cout << "Error al cargar rojo_espalda.png\n";
        }

        if (!texturaDerecha.loadFromFile("assets/images/rojo_derecha.png"))
        {
            std::cout << "Error al cargar rojo_derecha.png\n";
        }

        if (!texturaIzquierda.loadFromFile("assets/images/rojo_izquierda.png"))
        {
            std::cout << "Error al cargar rojo_izquierda.png\n";
        }

        direccion = ABAJO;
        numFrames = 7;

        sprite.setTexture(texturaFrente);

        // SPRITESHEET VERTICAL
        frameWidth = texturaFrente.getSize().x;          // 724
        frameHeight = texturaFrente.getSize().y / numFrames; // 2172 / 7

        sprite.setTextureRect(sf::IntRect(0, 0, frameWidth, frameHeight));
        sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
        sprite.setPosition(position);

        // Ajusta esto si se ve muy grande o muy chico
        sprite.setScale(0.50f, 0.50f);
    }

    void move(float offsetX, float offsetY, Direccion nuevaDireccion)
    {
        sprite.move(offsetX, offsetY);
        direccion = nuevaDireccion;
        caminando = true;
    }

    void update()
    {
        cambiarTextura();

        if (caminando)
        {
            if (clock.getElapsedTime().asSeconds() >= frameTime)
            {
                currentFrame = (currentFrame + 1) % numFrames;

                // AQUÍ LEEMOS EN VERTICAL
                sprite.setTextureRect(sf::IntRect(
                    0,
                    currentFrame * frameHeight,
                    frameWidth,
                    frameHeight
                ));

                clock.restart();
            }
        }
        else
        {
            currentFrame = 0;

            // quieto = primer frame
            sprite.setTextureRect(sf::IntRect(
                0,
                0,
                frameWidth,
                frameHeight
            ));
        }

        caminando = false;
    }

    void draw(sf::RenderWindow &window)
    {
        window.draw(sprite);
    }

private:
    sf::Sprite sprite;
    sf::Texture texturaFrente;
    sf::Texture texturaEspalda;
    sf::Texture texturaDerecha;
    sf::Texture texturaIzquierda;

    sf::Clock clock;

    Direccion direccion;
    bool caminando = false;

    int currentFrame = 0;
    int numFrames = 7;
    int frameWidth = 0;
    int frameHeight = 0;

    float frameTime = 0.03f;

    void cambiarTextura()
    {
        if (direccion == ABAJO)
        {
            sprite.setTexture(texturaFrente);
            frameWidth = texturaFrente.getSize().x;
            frameHeight = texturaFrente.getSize().y / numFrames;
        }
        else if (direccion == ARRIBA)
        {
            sprite.setTexture(texturaEspalda);
            frameWidth = texturaEspalda.getSize().x;
            frameHeight = texturaEspalda.getSize().y / numFrames;
        }
        else if (direccion == DERECHA)
        {
            sprite.setTexture(texturaDerecha);
            frameWidth = texturaDerecha.getSize().x;
            frameHeight = texturaDerecha.getSize().y / numFrames;
        }
        else if (direccion == IZQUIERDA)
        {
            sprite.setTexture(texturaIzquierda);
            frameWidth = texturaIzquierda.getSize().x;
            frameHeight = texturaIzquierda.getSize().y / numFrames;
        }

        sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Explosive-Knights");
    window.setFramerateLimit(60);

    Personaje caballeroRojo(sf::Vector2f(400, 300));
    float velocidad = 6.0f;

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
            caballeroRojo.move(0, -velocidad, ARRIBA);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            caballeroRojo.move(0, velocidad, ABAJO);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            caballeroRojo.move(-velocidad, 0, IZQUIERDA);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            caballeroRojo.move(velocidad, 0, DERECHA);
        }

        caballeroRojo.update();

        window.clear(sf::Color(35, 35, 45));
        caballeroRojo.draw(window);
        window.display();
    }

    return 0;
}