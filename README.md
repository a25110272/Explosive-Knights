# Explosive-Knights

📝 Descripción del Proyecto
Explosive Knights 💣🏰 es un videojuego de acción y estrategia en perspectiva top-down inspirado en la fórmula clásica de los arcades como Bomberman, pero adaptado a un entorno medieval oscuro. Desarrollado desde cero, el juego combina un apartado visual en pixel art con físicas en tiempo real e Inteligencia Artificial evolutiva para ofrecer una experiencia intensa, desafiante y con un alto grado de pulido técnico.

🎯 Objetivo del Juego
El objetivo principal depende del modo de juego seleccionado:

Modo Arcade (Campaña): Sobrevivir y limpiar las mazmorras derrotando a oleadas de enemigos letales a lo largo de 4 niveles, recolectando power-ups y superando al poderoso Jefe Final en un combate táctico de alta dificultad.

Modo Batalla (PvP): Eliminar a los demás jugadores en una arena cerrada mediante el uso estratégico de bombas y reacciones en cadena antes de que se agote el tiempo.

🎮 Controles
Flechas de dirección / WASD: Movimiento del Caballero (P1 / P2).

Barra Espaciadora / Tecla de Acción: Plantar bomba.

Choque Físico (con ítem Bota): Patear/Empujar bombas en la dirección del movimiento.

Teclado (Menús): Flechas Izquierda/Derecha para navegar opciones y Enter para confirmar selección.

⚙️ Mecánicas
Destrucción y Loot: Romper bloques del mapa para abrir caminos y revelar consumibles aleatorios.

Estadísticas de Personaje: Incremento dinámico de velocidad, rango de explosión (flama) y límite de bombas activas.

Progresión Persistente (Arcade): Las mejoras obtenidas se conservan al avanzar de nivel; solo se pierden si el jugador se queda sin vidas (Game Over).

Filtros de Colisión Óptimos: Los enemigos se traspasan entre sí para evitar atascos en pasillos, pero mantienen colisiones sólidas con el entorno, las bombas y el jugador.

Sistema de Drop al Morir: Al perder una vida, el jugador suelta parte de sus objetos mejorados en el mapa, abriendo la posibilidad de recuperarlos o que los robe el compañero.

🏆 Características
Modo Cooperativo Local: Soporte completo para 1 o 2 jugadores simultáneos en la misma pantalla.

IA Progresiva y Adaptativa: Los enemigos comunes cambian su comportamiento de patrullas aleatorias (Nivel 1 y 2) a caza activa del jugador (Nivel 3 y 4).

Jefe Final Avanzado: Jefe regido por una máquina de estados finitos que evade explosiones, persigue objetivos, es inmune a sus propias bombas y contraataca pateando proyectiles.

Interfaz de Usuario (UI) Pixel-Perfect: Menús de selección inmersivos y pantallas de victoria (Congratulations_arcade.png) adaptadas al arte nativo del juego.

👥 Equipo
Integrante 1: Diego Aldair Escoto Del Rio (@Diego-DelRio)

Integrante 2: Luis Alejandro Hernandez (@a25110272)

🛠️ Tecnologías
Motor/Framework: Framework multimedia SFML (Simple and Fast Multimedia Library).

Motor de Físicas: Box2D (Gestión completa de cuerpos rígidos, colisiones y fuerzas).

Lenguaje: C++ moderno.

📜 Créditos
Assets de terceros: Sprites y hojas de animaciones en Pixel Art de temática medieval (Caballeros, Mazmorras e Ítems).

Referencias e inspiraciones: Mecánicas clásicas de Bomberman (Hudson Soft).

Agradecimientos: Al foro y comunidad estudiantil por el espacio de difusión y retroalimentación técnica.