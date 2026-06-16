#include "GestorArcade.hpp"

#include "Mapa.hpp"
#include "Puerta.hpp"

#include <iostream>

GestorArcade::GestorArcade()
    : nivelActual(1),
      cantidadJugadores(1),
      puertaAbierta(false),
      bossPendiente(false),
      bossActivo(false),
      solicitudAvance(false),
      victoriaArcade(false)
{
}

void GestorArcade::iniciar(int jugadores)
{
    nivelActual = 1;
    cantidadJugadores = jugadores < 2 ? 1 : 2;
    puertaAbierta = false;
    bossPendiente = false;
    bossActivo = false;
    solicitudAvance = false;
    victoriaArcade = false;
}

void GestorArcade::cargarNivelActual(Mapa& mapa, PhysicsSpace& physics, Knight& jugador1, Knight& jugador2,
                                     std::vector<Enemigo>& enemigos, std::vector<Boss>& jefes,
                                     std::vector<Bomba>& bombas, std::vector<Explosion>& explosiones,
                                     std::vector<PowerUp>& items, Puerta& puerta, int& corazonesSpawneados)
{
    for (auto& bomba : bombas)
    {
        bomba.destruirFisica();
    }

    for (auto& enemigo : enemigos)
    {
        enemigo.destruir(physics);
    }

    for (auto& jefe : jefes)
    {
        jefe.destruir(physics);
    }

    bombas.clear();
    enemigos.clear();
    explosiones.clear();
    items.clear();
    jefes.clear();
    corazonesSpawneados = 0;

    mapa.cargarFondo(obtenerFondoNivel());
    mapa.setTemaObjetos(obtenerTemaObjetosNivel());
    mapa.inicializarGrid();
    mapa.generarFisicas(physics);

    jugador1.reiniciar(96.0f, 96.0f);
    if (cantidadJugadores == 2)
    {
        jugador2.reiniciar(864.0f, 736.0f);
    }
    else
    {
        jugador2.reiniciar(-500.0f, -500.0f);
    }

    if (nivelActual < 4)
    {
        colocarPuertaDeNivel(mapa, physics, puerta);
    }
    else
    {
        puerta.ocultar();
    }

    llenarItemsIniciales(mapa, items, corazonesSpawneados);

    puertaAbierta = false;
    bossPendiente = false;
    bossActivo = false;
    solicitudAvance = false;
    victoriaArcade = false;

    std::cout << "ARCADE NIVEL " << nivelActual << " cargado: " << getNombreNivelActual() << std::endl;
}

void GestorArcade::update(Mapa& mapa, PhysicsSpace& physics, std::vector<Enemigo>& enemigos,
                          std::vector<Boss>& jefes, Puerta& puerta)
{
    if (victoriaArcade)
    {
        return;
    }

    if (nivelActual < 4)
    {
        if (!puertaAbierta && enemigos.empty())
        {
            puertaAbierta = true;
            std::cout << "Puerta arcade abierta" << std::endl;
        }

        if (!puerta.estaVisible() && mapa.obtenerTipoCelda(puerta.getFila(), puerta.getColumna()) == Mapa::VACIO)
        {
            if (puertaAbierta)
            {
                puerta.abrir();
            }
            else
            {
                puerta.revelar();
            }
        }
        else if (puerta.estaVisible() && puertaAbierta && !puerta.estaAbierta())
        {
            puerta.abrir();
        }
        return;
    }

    if (!bossActivo && !bossPendiente && enemigos.empty())
    {
        mapa.limpiarBloquesDestructibles(physics);
        bossPendiente = true;
        float bossX = 7.0f * 64.0f + 32.0f;
        float bossY = 6.0f * 64.0f + 32.0f;
        jefes.emplace_back(bossX, bossY, physics, "assets/images/Negro_spritesheet.png");
        bossActivo = true;
        std::cout << "Jefe Final preparado. Bomba de jefe: assets/images/Bomba_Jefe.png" << std::endl;
    }

    if (bossActivo && jefes.empty())
    {
        victoriaArcade = true;
    }
}

void GestorArcade::solicitarAvanceNivel()
{
    if (nivelActual < 4)
    {
        solicitudAvance = true;
    }
}

bool GestorArcade::consumirSolicitudAvance()
{
    if (!solicitudAvance)
    {
        return false;
    }

    solicitudAvance = false;
    nivelActual++;
    if (nivelActual > 4)
    {
        victoriaArcade = true;
        return false;
    }

    return true;
}

bool GestorArcade::consumirVictoriaArcade()
{
    if (!victoriaArcade)
    {
        return false;
    }

    victoriaArcade = false;
    return true;
}

bool GestorArcade::esCooperativo() const
{
    return cantidadJugadores == 2;
}

int GestorArcade::getNivelActual() const
{
    return nivelActual;
}

int GestorArcade::getCantidadJugadores() const
{
    return cantidadJugadores;
}

std::string GestorArcade::getNombreNivelActual() const
{
    switch (nivelActual)
    {
    case 1:
        return "Mapa rojo";
    case 2:
        return "Mapa claro";
    case 3:
        return "Mapa verde";
    case 4:
        return "Mapa negro";
    default:
        return "Mapa arcade";
    }
}

std::string GestorArcade::obtenerFondoNivel() const
{
    switch (nivelActual)
    {
    case 1:
        return "assets/images/Mapa rojo.png";
    case 2:
        return "assets/images/Mapa claro.png";
    case 3:
        return "assets/images/Mapa verde.png";
    case 4:
        return "assets/images/Mapa negro.png";
    default:
        return "assets/images/Mapa.png";
    }
}

int GestorArcade::obtenerTemaObjetosNivel() const
{
    return nivelActual - 1;
}

void GestorArcade::colocarPuertaDeNivel(Mapa& mapa, PhysicsSpace& physics, Puerta& puerta)
{
    const int filaPuerta = 5;
    const int columnaPuerta = 7;
    mapa.colocarBloqueDestructible(filaPuerta, columnaPuerta, physics);
    puerta.colocar(filaPuerta, columnaPuerta);
}
