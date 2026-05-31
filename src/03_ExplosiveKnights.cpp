#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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
            std::cout << "Error: no se pudo cargar assets/images/rojo_frente.png" << std::endl;
        }

        if (!texturaEspalda.loadFromFile("assets/images/rojo_espalda.png"))
        {
            std::cout << "Error: no se pudo cargar assets/images/rojo_espalda.png" << std::endl;
        }

        if (!texturaDerecha.loadFromFile("assets/images/rojo_derecha.png"))
        {
            std::cout << "Error: no se pudo cargar assets/images/rojo_derecha.png" << std::endl;
        }

        if (!texturaIzquierda.loadFromFile("assets/images/rojo_izquierda.png"))
        {
            std::cout << "Error: no se pudo cargar assets/images/rojo_izquierda.png" << std::endl;
        }

        direccion = ABAJO;

        sprite.setTexture(texturaFrente);

        numFrames = 4;
        frameWidth = texturaFrente.getSize().x / numFrames;
        frameHeight = texturaFrente.getSize().y;

        sprite.setTextureRect(sf::IntRect(0, 0, frameWidth, frameHeight));
        sprite.setPosition(position);

        // Cambia la escala si se ve muy grande o muy pequeño
        sprite.setScale(2.0f, 2.0f);
    }

    void move(float offsetX, float offsetY, Direccion nuevaDireccion)
    {
        sprite.move(offsetX, offsetY);
        direccion = nuevaDireccion;
        caminando = true;
    }

    void draw(sf::RenderWindow &window)
    {
        window.draw(sprite);
    }

    void update()
    {
        cambiarTextura();

        if (caminando)
        {
            if (clock.getElapsedTime().asSeconds() >= frameTime)
            {
                currentFrame = (currentFrame + 1) % numFrames;

                sprite.setTextureRect(sf::IntRect(
                    currentFrame * frameWidth,
                    0,
                    frameWidth,
                    frameHeight
                ));

                clock.restart();
            }
        }
        else
        {
            currentFrame = 0;

            sprite.setTextureRect(sf::IntRect(
                0,
                0,
                frameWidth,
                frameHeight
            ));
        }

        caminando = false;
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

    float frameTime = 0.12f;
    int currentFrame = 0;
    int numFrames = 4;
    int frameWidth = 0;
    int frameHeight = 0;

    void cambiarTextura()
    {
        if (direccion == ABAJO)
        {
            sprite.setTexture(texturaFrente);
            frameWidth = texturaFrente.getSize().x / numFrames;
            frameHeight = texturaFrente.getSize().y;
        }
        else if (direccion == ARRIBA)
        {
            sprite.setTexture(texturaEspalda);
            frameWidth = texturaEspalda.getSize().x / numFrames;
            frameHeight = texturaEspalda.getSize().y;
        }
        else if (direccion == DERECHA)
        {
            sprite.setTexture(texturaDerecha);
            frameWidth = texturaDerecha.getSize().x / numFrames;
            frameHeight = texturaDerecha.getSize().y;
        }
        else if (direccion == IZQUIERDA)
        {
            sprite.setTexture(texturaIzquierda);
            frameWidth = texturaIzquierda.getSize().x / numFrames;
            frameHeight = texturaIzquierda.getSize().y;
        }
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Explosive-Knights");
    window.setFramerateLimit(60);

    Personaje caballeroRojo(sf::Vector2f(400, 300));

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
            caballeroRojo.move(0, -velocidad, ARRIBA);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            caballeroRojo.move(0, velocidad, ABAJO);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            caballeroRojo.move(-velocidad, 0, IZQUIERDA);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
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