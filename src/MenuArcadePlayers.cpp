#include "MenuArcadePlayers.hpp"

#include <iostream>

MenuArcadePlayers::MenuArcadePlayers()
    : fuenteCargada(false), confirmarSeleccion(false), opcionActual(0)
{
}

bool MenuArcadePlayers::init()
{
    fuenteCargada = fuente.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
    if (!fuenteCargada)
    {
        std::cout << "Aviso: No se pudo cargar fuente para MenuArcadePlayers" << std::endl;
    }

    fondo.setSize(sf::Vector2f(960.0f, 832.0f));
    fondo.setFillColor(sf::Color(18, 20, 28));

    selector.setSize(sf::Vector2f(320.0f, 54.0f));
    selector.setOrigin(160.0f, 27.0f);
    selector.setFillColor(sf::Color(255, 220, 40, 55));
    selector.setOutlineColor(sf::Color::Yellow);
    selector.setOutlineThickness(3.0f);

    if (fuenteCargada)
    {
        titulo.setFont(fuente);
        titulo.setCharacterSize(52);
        titulo.setFillColor(sf::Color::Yellow);
        titulo.setOutlineColor(sf::Color::Black);
        titulo.setOutlineThickness(4.0f);
        titulo.setString("MODO ARCADE");

        opcionUno.setFont(fuente);
        opcionUno.setCharacterSize(34);
        opcionUno.setFillColor(sf::Color::White);
        opcionUno.setOutlineColor(sf::Color::Black);
        opcionUno.setOutlineThickness(3.0f);
        opcionUno.setString("1 JUGADOR");

        opcionDos.setFont(fuente);
        opcionDos.setCharacterSize(34);
        opcionDos.setFillColor(sf::Color::White);
        opcionDos.setOutlineColor(sf::Color::Black);
        opcionDos.setOutlineThickness(3.0f);
        opcionDos.setString("2 JUGADORES");

        instruccion.setFont(fuente);
        instruccion.setCharacterSize(22);
        instruccion.setFillColor(sf::Color::White);
        instruccion.setOutlineColor(sf::Color::Black);
        instruccion.setOutlineThickness(2.0f);
        instruccion.setString("Usa izquierda/derecha y ENTER");
    }

    actualizarTextos();
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
        actualizarTextos();
    }
    else if (event.key.code == sf::Keyboard::Return)
    {
        confirmarSeleccion = true;
    }
}

void MenuArcadePlayers::draw(sf::RenderWindow& window)
{
    window.draw(fondo);
    window.draw(selector);

    if (fuenteCargada)
    {
        window.draw(titulo);
        window.draw(opcionUno);
        window.draw(opcionDos);
        window.draw(instruccion);
    }
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

void MenuArcadePlayers::actualizarTextos()
{
    selector.setPosition(opcionActual == 0 ? 300.0f : 660.0f, 430.0f);

    if (!fuenteCargada)
    {
        return;
    }

    centrarTexto(titulo, 480.0f, 220.0f);
    centrarTexto(opcionUno, 300.0f, 410.0f);
    centrarTexto(opcionDos, 660.0f, 410.0f);
    centrarTexto(instruccion, 480.0f, 720.0f);
}

void MenuArcadePlayers::centrarTexto(sf::Text& texto, float x, float y)
{
    sf::FloatRect bounds = texto.getLocalBounds();
    texto.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    texto.setPosition(x, y);
}
