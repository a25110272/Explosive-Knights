#ifndef GESTOR_ARCADE_HPP
#define GESTOR_ARCADE_HPP

#include <string>
#include <utility>
#include <vector>

class Mapa;
class PhysicsSpace;
class Knight;
class Bomba;
class Explosion;
class PowerUp;
class Enemigo;
class Boss;
class Puerta;

class GestorArcade
{
public:
    GestorArcade();

    void iniciar(int jugadores);
    void cargarNivelActual(Mapa& mapa, PhysicsSpace& physics, Knight& jugador1, Knight& jugador2,
                           std::vector<Enemigo>& enemigos, std::vector<Boss>& jefes,
                           std::vector<Bomba>& bombas, std::vector<Explosion>& explosiones,
                           std::vector<PowerUp>& items, Puerta& puerta, int& corazonesSpawneados);
    void update(Mapa& mapa, PhysicsSpace& physics, std::vector<Enemigo>& enemigos,
                std::vector<Boss>& jefes, Puerta& puerta);
    void solicitarAvanceNivel();
    bool consumirSolicitudAvance();
    bool consumirVictoriaArcade();
    bool esCooperativo() const;
    int getNivelActual() const;
    int getCantidadJugadores() const;
    std::string getNombreNivelActual() const;

private:
    std::string obtenerFondoNivel() const;
    int obtenerTemaObjetosNivel() const;
    void colocarPuertaDeNivel(Mapa& mapa, PhysicsSpace& physics, Puerta& puerta);
    void spawnearEnemigosNivel(Mapa& mapa, PhysicsSpace& physics, std::vector<Enemigo>& enemigos);
    std::string obtenerRutaEnemigo(int tipo) const;
    bool celdaSpawnEnemigoValida(Mapa& mapa, int fila, int columna,
                                  const std::vector<std::pair<int, int>>& ocupadas) const;

    int nivelActual;
    int cantidadJugadores;
    bool puertaAbierta;
    bool bossPendiente;
    bool bossActivo;
    bool solicitudAvance;
    bool victoriaArcade;
    bool vidaSpawneadaNivel;
};

#endif
