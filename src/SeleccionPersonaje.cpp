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
    : personajeSeleccionadoP1(0),
      personajeSeleccionadoP2(2),
      confirmarSeleccion(false),
      fondoCargado(false),
      fuenteCargada(false),
      modoMultijugador(false)
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

    fuenteCargada = fuente.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    fondoFallback.setSize(sf::Vector2f(SELECCION_WINDOW_WIDTH, SELECCION_WINDOW_HEIGHT));
    fondoFallback.setFillColor(sf::Color(22, 24, 34));

    selectorP1.setSize(sf::Vector2f(200.0f, 420.0f));
    selectorP1.setFillColor(sf::Color::Transparent);
    selectorP1.setOutlineColor(sf::Color(255, 214, 64));
    selectorP1.setOutlineThickness(5.0f);

    selectorP2.setSize(sf::Vector2f(200.0f, 420.0f));
    selectorP2.setFillColor(sf::Color::Transparent);
    selectorP2.setOutlineColor(sf::Color(64, 190, 255));
    selectorP2.setOutlineThickness(5.0f);

    if (fuenteCargada)
    {
        etiquetaP1.setFont(fuente);
        etiquetaP1.setString("P1");
        etiquetaP1.setCharacterSize(34);
        etiquetaP1.setFillColor(sf::Color(255, 230, 80));
        etiquetaP1.setOutlineColor(sf::Color::Black);
        etiquetaP1.setOutlineThickness(3.0f);

        etiquetaP2.setFont(fuente);
        etiquetaP2.setString("P2");
        etiquetaP2.setCharacterSize(34);
        etiquetaP2.setFillColor(sf::Color(90, 210, 255));
        etiquetaP2.setOutlineColor(sf::Color::Black);
        etiquetaP2.setOutlineThickness(3.0f);

        instrucciones.setFont(fuente);
        instrucciones.setCharacterSize(20);
        instrucciones.setFillColor(sf::Color(235, 235, 235));
        instrucciones.setOutlineColor(sf::Color::Black);
        instrucciones.setOutlineThickness(2.0f);
    }

    update();
    return fondoCargado;
}

void SeleccionPersonaje::setModoMultijugador(bool activo)
{
    modoMultijugador = activo;
    confirmarSeleccion = false;
    update();
}

void SeleccionPersonaje::handleInput(sf::Event& event)
{
    if (event.type != sf::Event::KeyPressed)
    {
        return;
    }

    if (modoMultijugador)
    {
        if (event.key.code == sf::Keyboard::A)
        {
            moverSeleccion(personajeSeleccionadoP1, -1);
        }
        else if (event.key.code == sf::Keyboard::D)
        {
            moverSeleccion(personajeSeleccionadoP1, 1);
        }
        else if (event.key.code == sf::Keyboard::Left)
        {
            moverSeleccion(personajeSeleccionadoP2, -1);
        }
        else if (event.key.code == sf::Keyboard::Right)
        {
            moverSeleccion(personajeSeleccionadoP2, 1);
        }
        else if (event.key.code == sf::Keyboard::Return)
        {
            confirmarSeleccion = true;
        }
    }
    else
    {
        if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A)
        {
            moverSeleccion(personajeSeleccionadoP1, -1);
        }
        else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D)
        {
            moverSeleccion(personajeSeleccionadoP1, 1);
        }
        else if (event.key.code == sf::Keyboard::Return)
        {
            confirmarSeleccion = true;
        }
    }
}

void SeleccionPersonaje::update()
{
    selectorP1.setPosition(SELECCION_SELECTOR_X[personajeSeleccionadoP1], SELECCION_SELECTOR_Y);
    selectorP2.setPosition(SELECCION_SELECTOR_X[personajeSeleccionadoP2], SELECCION_SELECTOR_Y);

    if (fuenteCargada)
    {
        centrarTexto(etiquetaP1, SELECCION_SELECTOR_X[personajeSeleccionadoP1] + 100.0f, SELECCION_SELECTOR_Y - 48.0f);

        float offsetY = personajeSeleccionadoP1 == personajeSeleccionadoP2 ? -10.0f : 0.0f;
        centrarTexto(etiquetaP2, SELECCION_SELECTOR_X[personajeSeleccionadoP2] + 100.0f, SELECCION_SELECTOR_Y - 48.0f + offsetY);

        if (modoMultijugador)
        {
            instrucciones.setString("P1: A/D  |  P2: Flechas  |  Enter para jugar");
        }
        else
        {
            instrucciones.setString("A/D o Flechas para elegir  |  Enter para jugar");
        }
        centrarTexto(instrucciones, SELECCION_WINDOW_WIDTH / 2.0f, 782.0f);
    }
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

    window.draw(selectorP1);
    if (modoMultijugador)
    {
        window.draw(selectorP2);
    }

    if (fuenteCargada)
    {
        window.draw(etiquetaP1);
        if (modoMultijugador)
        {
            window.draw(etiquetaP2);
        }
        window.draw(instrucciones);
    }
}

int SeleccionPersonaje::getPersonajeSeleccionado() const
{
    return personajeSeleccionadoP1;
}

int SeleccionPersonaje::getPersonajeSeleccionadoP1() const
{
    return personajeSeleccionadoP1;
}

int SeleccionPersonaje::getPersonajeSeleccionadoP2() const
{
    return personajeSeleccionadoP2;
}

std::string SeleccionPersonaje::getRutaTexturaPersonaje(int personaje) const
{
    const std::string rutasPersonajes[4] = {
        "assets/images/verde_spritesheet.png",
        "assets/images/Rojo_spritesheet.png",
        "assets/images/Azul_spritesheet.png",
        "assets/images/Negro_spritesheet.png"
    };

    if (personaje < 0 || personaje >= 4)
    {
        return rutasPersonajes[0];
    }

    return rutasPersonajes[personaje];
}

bool SeleccionPersonaje::seleccionConfirmada() const
{
    return confirmarSeleccion;
}

void SeleccionPersonaje::limpiarConfirmacion()
{
    confirmarSeleccion = false;
}

void SeleccionPersonaje::moverSeleccion(int& personaje, int direccion)
{
    personaje = (personaje + direccion + 4) % 4;
    update();
}

void SeleccionPersonaje::centrarTexto(sf::Text& texto, float x, float y)
{
    sf::FloatRect bounds = texto.getLocalBounds();
    texto.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    texto.setPosition(x, y);
}
