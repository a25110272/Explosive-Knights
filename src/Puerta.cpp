#include "Puerta.hpp"

#include <iostream>

Puerta::Puerta()
    : fila(1),
      columna(1),
      visible(false),
      abierta(false),
      texturaCerradaCargada(false),
      texturaAbiertaCargada(false)
{
}

bool Puerta::init()
{
    texturaCerradaCargada = texturaCerrada.loadFromFile("assets/images/Puerta cerrada.png");
    if (!texturaCerradaCargada)
    {
        std::cout << "Aviso: No se pudo cargar assets/images/Puerta cerrada.png" << std::endl;
    }

    texturaAbiertaCargada = texturaAbierta.loadFromFile("assets/images/Puerta abierta.png");
    if (!texturaAbiertaCargada)
    {
        std::cout << "Aviso: No se pudo cargar assets/images/Puerta abierta.png" << std::endl;
    }

    actualizarSprite();
    return texturaCerradaCargada || texturaAbiertaCargada;
}

void Puerta::colocar(int nuevaFila, int nuevaColumna)
{
    fila = nuevaFila;
    columna = nuevaColumna;
    visible = false;
    abierta = false;
    actualizarSprite();
}

void Puerta::revelar()
{
    visible = true;
    actualizarSprite();
}

void Puerta::abrir()
{
    abierta = true;
    visible = true;
    actualizarSprite();
}

void Puerta::cerrar()
{
    abierta = false;
    actualizarSprite();
}

void Puerta::ocultar()
{
    visible = false;
    abierta = false;
}

void Puerta::draw(sf::RenderWindow& window)
{
    if (!visible)
    {
        return;
    }

    if ((abierta && texturaAbiertaCargada) || (!abierta && texturaCerradaCargada))
    {
        window.draw(sprite);
        return;
    }

    sf::RectangleShape fallback(sf::Vector2f(64.0f, 64.0f));
    fallback.setOrigin(32.0f, 32.0f);
    fallback.setPosition(columna * 64.0f + 32.0f, fila * 64.0f + 32.0f);
    fallback.setFillColor(abierta ? sf::Color(80, 220, 120) : sf::Color(90, 60, 160));
    fallback.setOutlineColor(sf::Color::Black);
    fallback.setOutlineThickness(2.0f);
    window.draw(fallback);
}

bool Puerta::estaAbierta() const
{
    return abierta && visible;
}

bool Puerta::estaVisible() const
{
    return visible;
}

bool Puerta::jugadorEstaEncima(float pixelX, float pixelY) const
{
    if (!estaAbierta())
    {
        return false;
    }

    int filaJugador = static_cast<int>(pixelY / 64.0f);
    int columnaJugador = static_cast<int>(pixelX / 64.0f);
    return filaJugador == fila && columnaJugador == columna;
}

int Puerta::getFila() const
{
    return fila;
}

int Puerta::getColumna() const
{
    return columna;
}

void Puerta::actualizarSprite()
{
    if (abierta && texturaAbiertaCargada)
    {
        sprite.setTexture(texturaAbierta, true);
    }
    else if (texturaCerradaCargada)
    {
        sprite.setTexture(texturaCerrada, true);
    }
    else
    {
        return;
    }

    sf::Vector2u size = sprite.getTexture()->getSize();
    if (size.x > 0 && size.y > 0)
    {
        sprite.setOrigin(size.x / 2.0f, size.y / 2.0f);
        sprite.setScale(64.0f / static_cast<float>(size.x), 64.0f / static_cast<float>(size.y));
    }

    sprite.setPosition(columna * 64.0f + 32.0f, fila * 64.0f + 32.0f);
}
