#ifndef MENU_HPP
#define MENU_HPP

#include <SFML/Graphics.hpp>

class Menu
{
public:
    Menu();

    bool init();
    void handleInput(sf::Event& event);
    void draw(sf::RenderWindow& window);
    int getOpcionSeleccionada() const;
    bool seleccionConfirmada() const;
    void limpiarConfirmacion();

private:
    void actualizarFlechas();

    sf::Texture texturaFondo;
    sf::Sprite fondo;
    sf::CircleShape flechaIzq;
    sf::CircleShape flechaDer;
    int opcionActual;
    bool fondoCargado;
    bool confirmarSeleccion;
};

#endif
