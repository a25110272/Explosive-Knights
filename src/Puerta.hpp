#ifndef PUERTA_HPP
#define PUERTA_HPP

#include <SFML/Graphics.hpp>

class Puerta
{
public:
    Puerta();

    bool init();
    void colocar(int fila, int columna);
    void revelar();
    void abrir();
    void cerrar();
    void ocultar();
    void draw(sf::RenderWindow& window);

    bool estaAbierta() const;
    bool estaVisible() const;
    bool jugadorEstaEncima(float pixelX, float pixelY) const;
    int getFila() const;
    int getColumna() const;

private:
    void actualizarSprite();

    int fila;
    int columna;
    bool visible;
    bool abierta;
    bool texturaCerradaCargada;
    bool texturaAbiertaCargada;
    sf::Texture texturaCerrada;
    sf::Texture texturaAbierta;
    sf::Sprite sprite;
};

#endif
