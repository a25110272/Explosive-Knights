#ifndef SELECCION_PERSONAJE_HPP
#define SELECCION_PERSONAJE_HPP

#include <SFML/Graphics.hpp>
#include <string>

class SeleccionPersonaje
{
public:
    SeleccionPersonaje();

    bool init();
    void setModoMultijugador(bool activo);
    void handleInput(sf::Event& event);
    void update();
    void draw(sf::RenderWindow& window);

    int getPersonajeSeleccionado() const;
    int getPersonajeSeleccionadoP1() const;
    int getPersonajeSeleccionadoP2() const;
    std::string getRutaTexturaPersonaje(int personaje) const;
    bool seleccionConfirmada() const;
    void limpiarConfirmacion();

private:
    void moverSeleccion(int& personaje, int direccion);
    void centrarTexto(sf::Text& texto, float x, float y);

    sf::Texture texturaFondo;
    sf::Sprite fondo;
    sf::RectangleShape selectorP1;
    sf::RectangleShape selectorP2;
    sf::RectangleShape fondoFallback;
    sf::Font fuente;
    sf::Text etiquetaP1;
    sf::Text etiquetaP2;
    sf::Text instrucciones;
    int jugadorEligiendo;
    int personajeSeleccionadoP1;
    int personajeSeleccionadoP2;
    bool confirmarSeleccion;
    bool fondoCargado;
    bool fuenteCargada;
    bool modoMultijugador;
};

#endif
