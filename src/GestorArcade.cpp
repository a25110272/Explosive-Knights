#include "GestorArcade.hpp"

#include "Mapa.hpp"
#include "Puerta.hpp"

#include <cstdlib>
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

    int vidasP1 = nivelActual == 1 ? 3 : jugador1.getVidas();
    int vidasP2 = nivelActual == 1 ? 3 : jugador2.getVidas();

    jugador1.reiniciar(96.0f, 96.0f);
    jugador1.setVidas(vidasP1);
    if (vidasP1 > 0)
    {
        jugador1.activarArcade();
    }
    else
    {
        jugador1.desactivarArcade();
    }

    if (cantidadJugadores == 2)
    {
        jugador2.reiniciar(864.0f, 736.0f);
        jugador2.setVidas(vidasP2);
        if (vidasP2 > 0)
        {
            jugador2.activarArcade();
        }
        else
        {
            jugador2.desactivarArcade();
        }
    }
    else
    {
        jugador2.reiniciar(-500.0f, -500.0f);
        jugador2.desactivarArcade();
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
    spawnearEnemigosNivel(mapa, physics, enemigos);

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

void GestorArcade::spawnearEnemigosNivel(Mapa& mapa, PhysicsSpace& physics, std::vector<Enemigo>& enemigos)
{
    struct GrupoEnemigos
    {
        int tipo;
        int cantidad;
    };

    std::vector<GrupoEnemigos> grupos;
    if (nivelActual == 1)
    {
        grupos = {{1, 3}, {2, 2}};
    }
    else if (nivelActual == 2)
    {
        grupos = {{3, 3}, {4, 3}};
    }
    else if (nivelActual == 3)
    {
        grupos = {{5, 4}, {6, 3}};
    }
    else
    {
        grupos = {{7, 4}, {8, 4}};
    }

    std::vector<std::pair<int, int>> ocupadas;
    for (const auto& grupo : grupos)
    {
        for (int i = 0; i < grupo.cantidad; i++)
        {
            bool creado = false;
            for (int intento = 0; intento < 200 && !creado; intento++)
            {
                int fila = 1 + rand() % 11;
                int columna = 1 + rand() % 13;

                if (!celdaSpawnEnemigoValida(mapa, fila, columna, ocupadas))
                {
                    continue;
                }

                float x = columna * 64.0f + 32.0f;
                float y = fila * 64.0f + 32.0f;
                enemigos.emplace_back(x, y, physics, obtenerRutaEnemigo(grupo.tipo));
                ocupadas.push_back({fila, columna});
                creado = true;
            }
        }
    }

    std::cout << "Enemigos arcade spawneados: " << enemigos.size() << std::endl;
}

std::string GestorArcade::obtenerRutaEnemigo(int tipo) const
{
    const std::string rutas[8] = {
        "assets/images/Enemigo_01_Nivel 1.png",
        "assets/images/Enemigo_02_Nivel 1.png",
        "assets/images/Enemigo_03_Nivel 2.png",
        "assets/images/Enemigo_04_Nivel 2.png",
        "assets/images/Enemigo_05_Nivel 3.png",
        "assets/images/Enemigo_06_Nivel 3.png",
        "assets/images/Enemigo_07_Nivel 4.png",
        "assets/images/Enemigo_08_Nivel 4.png"
    };

    if (tipo < 1 || tipo > 8)
    {
        return rutas[0];
    }

    return rutas[tipo - 1];
}

bool GestorArcade::celdaSpawnEnemigoValida(Mapa& mapa, int fila, int columna,
                                           const std::vector<std::pair<int, int>>& ocupadas) const
{
    if (mapa.obtenerTipoCelda(fila, columna) != Mapa::VACIO)
    {
        return false;
    }

    for (const auto& ocupada : ocupadas)
    {
        if (ocupada.first == fila && ocupada.second == columna)
        {
            return false;
        }
    }

    int distanciaP1 = std::abs(fila - 1) + std::abs(columna - 1);
    int distanciaP2 = std::abs(fila - 11) + std::abs(columna - 13);
    return distanciaP1 >= 5 && distanciaP2 >= 5;
}
