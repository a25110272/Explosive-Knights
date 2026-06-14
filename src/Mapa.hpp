#ifndef MAPA_HPP
#define MAPA_HPP

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <map>
#include <utility>
#include <vector>

class PhysicsSpace;
class PowerUp;

class Mapa
{
public:
    static const int VACIO = 0;
    static const int INDESTRUCTIBLE = 1;
    static const int DESTRUCTIBLE = 2;

    Mapa();
    ~Mapa();

    void inicializarGrid();
    void generarFisicas(PhysicsSpace& physics);
    void draw(sf::RenderWindow& window);
    int obtenerTipoCelda(int fila, int columna) const;
    void destruirBloque(int fila, int columna, std::vector<PowerUp>* pItems = nullptr);
    int getCellType(int fila, int col) const;

private:
    void limpiarFisicas();
    void calcularRectsObjetos(const sf::Image& imagen);
    int obtenerFilaTema(int fila, int columna) const;
    int obtenerColumnaObjeto(int fila, int columna, int tipo) const;
    void dibujarObjetoMapa(sf::RenderWindow& window, int fila, int columna, int tipo);

    int grid[13][15];
    std::map<std::pair<int, int>, b2BodyId> bodiesMap;
    sf::Texture texturaMapaBase;
    sf::Texture texturaObjetosMapa;
    std::vector<sf::IntRect> rectsObjetos;
    bool mapaBaseCargado;
    bool objetosMapaCargados;
};

#endif
