#ifndef SELECCION_PERSONAJE_HPP
#define SELECCION_PERSONAJE_HPP

#include <SFML/Graphics.hpp>

class SeleccionPersonaje
{
public:
    SeleccionPersonaje();

    bool init();
    void handleInput(sf::Event& event);
    void update();
    void draw(sf::RenderWindow& window);

    int getPersonajeSeleccionado() const;
    bool seleccionConfirmada() const;
    void limpiarConfirmacion();

private:
    sf::Texture texturaFondo;
    sf::Sprite fondo;
    sf::RectangleShape selector;
    sf::RectangleShape fondoFallback;
    int personajeSeleccionado;
    bool confirmarSeleccion;
    bool fondoCargado;
};

#endif
