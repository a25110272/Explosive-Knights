#ifndef KNIGHT_HPP
#define KNIGHT_HPP

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <memory>
#include <string>
#include <vector>

class PhysicsSpace;
class Mapa;
class Bomba;
class Personaje;

class Knight
{
public:
    Knight(sf::Vector2f position, PhysicsSpace& physics,
           const std::string& rutaTextura = "assets/images/Verde_spritesheet.png",
           int idJugador = 1);
    ~Knight();

    void handleInput();
    void handleInput(sf::Keyboard::Key teclaArriba,
                     sf::Keyboard::Key teclaAbajo,
                     sf::Keyboard::Key teclaDerecha,
                     sf::Keyboard::Key teclaIzquierda);
    void syncWithPhysics();
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);

    void plantarBomba(Mapa& mapa, std::vector<Bomba>& bombas, PhysicsSpace& physics);
    void recolectarItem(int tipo, bool modoVersus = false);
    bool morir(PhysicsSpace& physics, bool conservarMejoras = false);
    bool recibirDano(PhysicsSpace& physics, bool conservarMejoras = false);
    void cambiarSprite(const std::string& ruta);
    void configurarBomba(const std::string& ruta, sf::Color color);
    void reiniciar(float x, float y);
    void reiniciarRonda(float x, float y);
    void moverA(float x, float y);

    b2BodyId getBodyId();
    int getRangoFuego() const;
    int getMaxBombas() const;
    int getVidas() const;
    void setVidas(int nuevasVidas);
    int getDireccionActual() const;
    bool puedePatearBombas() const;

private:
    std::unique_ptr<Personaje> personaje;
    b2BodyId bodyId;
    PhysicsSpace& physicsSpace;
    int direccionActual = 0;
    int maxBombas;
    int rangoFuego;
    float speed;
    float speedOriginal;
    sf::Vector2f posicionOriginal;
    int vidas;
    float tiempoInvulnerable;
    bool puedePatear;
    int idJugador;
    std::string rutaTexturaBomba;
};

#endif
