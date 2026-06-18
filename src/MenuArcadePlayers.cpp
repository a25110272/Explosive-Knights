#include "MenuArcadePlayers.hpp"

#include <iostream>

namespace
{
const sf::Vector2f MENU_ARCADE_SELECTOR_SIZE(240.0f, 60.0f);
const float MENU_ARCADE_SELECTOR_Y = 430.0f;
const float MENU_ARCADE_SELECTOR_1P_X = 330.0f;
const float MENU_ARCADE_SELECTOR_2P_X = 630.0f;
}

MenuArcadePlayers::MenuArcadePlayers()
    : fondoCargado(false), confirmarSeleccion(false), opcionActual(0)
{
}

bool MenuArcadePlayers::init()
{
    fondoCargado = texturaFondo.loadFromFile("assets/images/Players_arcade.png");
    if (!fondoCargado)
    {
        std::cout << "Aviso: No se pudo cargar assets/images/Players_arcade.png" << std::endl;
    }
    else
    {
        spriteFondo.setTexture(texturaFondo, true);
        sf::Vector2u size = texturaFondo.getSize();
        if (size.x > 0 && size.y > 0)
        {
            spriteFondo.setScale(
                960.0f / static_cast<float>(size.x),
                832.0f / static_cast<float>(size.y)
            );
        }
    }

    fondoFallback.setSize(sf::Vector2f(960.0f, 832.0f));
    fondoFallback.setFillColor(sf::Color(18, 20, 28));

    selector.setSize(MENU_ARCADE_SELECTOR_SIZE);
    selector.setOrigin(MENU_ARCADE_SELECTOR_SIZE.x / 2.0f, MENU_ARCADE_SELECTOR_SIZE.y / 2.0f);
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineColor(sf::Color::Yellow);
    selector.setOutlineThickness(5.0f);

    actualizarSelector();
    return true;
}

void MenuArcadePlayers::handleInput(sf::Event& event)
{
    if (event.type != sf::Event::KeyPressed)
    {
        return;
    }

    if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right)
    {
        opcionActual = 1 - opcionActual;
        actualizarSelector();
    }
    else if (event.key.code == sf::Keyboard::Return)
    {
        confirmarSeleccion = true;
    }
}

void MenuArcadePlayers::draw(sf::RenderWindow& window)
{
    if (fondoCargado)
    {
        window.draw(spriteFondo);
    }
    else
    {
        window.draw(fondoFallback);
    }

    window.draw(selector);
}

bool MenuArcadePlayers::seleccionConfirmada() const
{
    return confirmarSeleccion;
}

int MenuArcadePlayers::getCantidadJugadores() const
{
    return opcionActual + 1;
}

void MenuArcadePlayers::limpiarConfirmacion()
{
    confirmarSeleccion = false;
}

void MenuArcadePlayers::actualizarSelector()
{
    selector.setPosition(
        opcionActual == 0
            ? sf::Vector2f(MENU_ARCADE_SELECTOR_1P_X, MENU_ARCADE_SELECTOR_Y)
            : sf::Vector2f(MENU_ARCADE_SELECTOR_2P_X, MENU_ARCADE_SELECTOR_Y)
    );
}
