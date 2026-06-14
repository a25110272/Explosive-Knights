#include "SeleccionPersonaje.hpp"

#include <iostream>

const float SELECCION_WINDOW_WIDTH = 960.0f;
const float SELECCION_WINDOW_HEIGHT = 832.0f;
const float SELECCION_SELECTOR_Y = 210.0f;
const float SELECCION_SELECTOR_X[4] = {
    65.0f,
    270.0f,
    485.0f,
    695.0f
};

SeleccionPersonaje::SeleccionPersonaje()
    : personajeSeleccionado(0), confirmarSeleccion(false), fondoCargado(false)
{
}

bool SeleccionPersonaje::init()
{
    fondoCargado = texturaFondo.loadFromFile("assets/images/Personajes.png");
    if (fondoCargado)
    {
        fondo.setTexture(texturaFondo);
        sf::Vector2u size = texturaFondo.getSize();
        if (size.x > 0 && size.y > 0)
        {
            fondo.setScale(SELECCION_WINDOW_WIDTH / size.x, SELECCION_WINDOW_HEIGHT / size.y);
        }
    }
    else
    {
        std::cout << "Aviso: No se pudo cargar assets/images/TU_IMAGEN_AQUI.png" << std::endl;
    }

    fondoFallback.setSize(sf::Vector2f(SELECCION_WINDOW_WIDTH, SELECCION_WINDOW_HEIGHT));
    fondoFallback.setFillColor(sf::Color(22, 24, 34));

    selector.setSize(sf::Vector2f(200.0f, 420.0f));
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineColor(sf::Color::Yellow);
    selector.setOutlineThickness(5.0f);

    update();
    return fondoCargado;
}

void SeleccionPersonaje::handleInput(sf::Event& event)
{
    if (event.type != sf::Event::KeyPressed)
    {
        return;
    }

    if (event.key.code == sf::Keyboard::Left)
    {
        personajeSeleccionado = (personajeSeleccionado + 3) % 4;
        update();
    }
    else if (event.key.code == sf::Keyboard::Right)
    {
        personajeSeleccionado = (personajeSeleccionado + 1) % 4;
        update();
    }
    else if (event.key.code == sf::Keyboard::Return)
    {
        confirmarSeleccion = true;
    }
}

void SeleccionPersonaje::update()
{
    selector.setPosition(SELECCION_SELECTOR_X[personajeSeleccionado], SELECCION_SELECTOR_Y);
}

void SeleccionPersonaje::draw(sf::RenderWindow& window)
{
    if (fondoCargado)
    {
        window.draw(fondo);
    }
    else
    {
        window.draw(fondoFallback);
    }

    window.draw(selector);
}

int SeleccionPersonaje::getPersonajeSeleccionado() const
{
    return personajeSeleccionado;
}

bool SeleccionPersonaje::seleccionConfirmada() const
{
    return confirmarSeleccion;
}

void SeleccionPersonaje::limpiarConfirmacion()
{
    confirmarSeleccion = false;
}
