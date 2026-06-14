#include "Menu.hpp"

#include <iostream>

const float MENU_BASE_WIDTH = 1280.0f;
const float MENU_BASE_HEIGHT = 720.0f;
const float MENU_WINDOW_WIDTH = 960.0f;
const float MENU_WINDOW_HEIGHT = 832.0f;
const float MENU_ARROW_Y = 645.0f * (MENU_WINDOW_HEIGHT / MENU_BASE_HEIGHT);
const float MENU_ARROW_LEFT_X[3] = {
    355.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH),
    550.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH),
    755.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH)
};
const float MENU_ARROW_RIGHT_X[3] = {
    560.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH),
    755.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH),
    935.0f * (MENU_WINDOW_WIDTH / MENU_BASE_WIDTH)
};

Menu::Menu()
    : opcionActual(0), fondoCargado(false), confirmarSeleccion(false)
{
}

bool Menu::init()
{
    fondoCargado = texturaFondo.loadFromFile("assets/images/Pantalla de inicio.png");
    if (fondoCargado)
    {
        fondo.setTexture(texturaFondo);
        sf::Vector2u size = texturaFondo.getSize();
        if (size.x > 0 && size.y > 0)
        {
            fondo.setScale(960.0f / size.x, 832.0f / size.y);
        }
    }
    else
    {
        std::cout << "Aviso: No se pudo cargar assets/images/Pantalla de inicio.png" << std::endl;
    }

    flechaIzq.setRadius(18.0f);
    flechaIzq.setPointCount(3);
    flechaIzq.setOrigin(18.0f, 18.0f);
    flechaIzq.setFillColor(sf::Color::Yellow);
    flechaIzq.setOutlineThickness(2.0f);
    flechaIzq.setOutlineColor(sf::Color::Black);
    flechaIzq.setRotation(90.0f);

    flechaDer.setRadius(18.0f);
    flechaDer.setPointCount(3);
    flechaDer.setOrigin(18.0f, 18.0f);
    flechaDer.setFillColor(sf::Color::Yellow);
    flechaDer.setOutlineThickness(2.0f);
    flechaDer.setOutlineColor(sf::Color::Black);
    flechaDer.setRotation(-90.0f);

    actualizarFlechas();
    return true;
}

void Menu::handleInput(sf::Event& event)
{
    if (event.type != sf::Event::KeyPressed)
    {
        return;
    }

    if (event.key.code == sf::Keyboard::Left)
    {
        opcionActual = (opcionActual + 2) % 3;
        actualizarFlechas();
    }
    else if (event.key.code == sf::Keyboard::Right)
    {
        opcionActual = (opcionActual + 1) % 3;
        actualizarFlechas();
    }
    else if (event.key.code == sf::Keyboard::Return)
    {
        confirmarSeleccion = true;
    }
}

void Menu::draw(sf::RenderWindow& window)
{
    window.draw(fondo);
    window.draw(flechaIzq);
    window.draw(flechaDer);
}

int Menu::getOpcionSeleccionada() const
{
    return opcionActual;
}

bool Menu::seleccionConfirmada() const
{
    return confirmarSeleccion;
}

void Menu::limpiarConfirmacion()
{
    confirmarSeleccion = false;
}

void Menu::actualizarFlechas()
{
    flechaIzq.setPosition(MENU_ARROW_LEFT_X[opcionActual], MENU_ARROW_Y);
    flechaDer.setPosition(MENU_ARROW_RIGHT_X[opcionActual], MENU_ARROW_Y);
}
