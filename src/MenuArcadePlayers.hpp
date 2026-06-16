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
    void actualizarTextos();
    void centrarTexto(sf::Text& texto, float x, float y);

    sf::Font fuente;
    sf::Text titulo;
    sf::Text opcionUno;
    sf::Text opcionDos;
    sf::Text instruccion;
    sf::RectangleShape fondo;
    sf::RectangleShape selector;
    bool fuenteCargada;
    bool confirmarSeleccion;
    int opcionActual;
};

#endif
