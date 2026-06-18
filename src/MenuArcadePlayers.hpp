#ifndef MENU_ARCADE_PLAYERS_HPP
#define MENU_ARCADE_PLAYERS_HPP

#include <SFML/Graphics.hpp>

class MenuArcadePlayers
{
public:
    MenuArcadePlayers();

    bool init();
    void handleInput(sf::Event& event);
    void draw(sf::RenderWindow& window);
    bool seleccionConfirmada() const;
    int getCantidadJugadores() const;
    void limpiarConfirmacion();

private:
    void actualizarSelector();

    sf::Texture texturaFondo;
    sf::Sprite spriteFondo;
    sf::RectangleShape fondoFallback;
    sf::RectangleShape selector;
    bool fondoCargado;
    bool confirmarSeleccion;
    int opcionActual;
};

#endif
