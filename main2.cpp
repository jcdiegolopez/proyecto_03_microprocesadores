#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <random>

// Estructura para manejar los proyectiles de la nave
struct projectilePlayer {
    int x, y; // Posición del proyectil
};

// Estructura para manejar los proyectiles de los aliens
struct projectileAlien {
    int x, y; // Posición del proyectil
};

// Estructura para movimiento de los aliens
struct Alien {
    int x, y;
    bool direction;  // true: derecha, false: izquierda
};

// Variables globales compartidas
std::vector<Alien> aliens;                        // Lista de aliens
std::vector<projectilePlayer> projectilePlayers;  // Lista de proyectiles de la nave
std::vector<projectileAlien> projectileAliens;    // Lista de proyectiles de los aliens
pthread_mutex_t projectilePlayer_mutex;           // Mutex para proteger la lista de proyectiles de la nave
pthread_mutex_t projectileAlien_mutex;            // Mutex para proteger la lista de proyectiles de los aliens
pthread_mutex_t aliens_mutex;                     // Mutex para proteger la lista de aliens
bool fire_projectilePlayer = false;               // Bandera para indicar cuándo disparar un proyectil
int score = 0;                                    // Variable para almacenar el puntaje
int lives = 3;                                    // Vidas de la nave

// Variables relacionadas con la dificultad
int current_round = 1;  // Ronda actual
int aliens_per_round;   // Número de aliens por ronda
int total_rounds = 5;   // Número total de rondas
bool game_over = false; // Bandera para indicar el fin del juego
int mode = 0;           // Modo de juego (0: fácil, 1: difícil)

// Dimensiones de la pantalla
int max_y, max_x;
int limx1 = 1, limx2, limy1 = 2, limy2;

// Estructura de la nave
const char* nave[] = {" A", "/_\\"};
int x_nave;

// Estrucutra de los aliens
const char* alien = "/=\\";

// Tiempo del último disparo
auto last_shot_time = std::chrono::high_resolution_clock::now();

// Variables para patrón de movimiento
int stepLimit;
int counter;

// Función para inicializar la pantalla ncurses
void init_screen() {
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(100);
    timeout(0); 
    nodelay(stdscr, TRUE); 

    // Inicializar las dimensiones de la pantalla
    getmaxyx(stdscr, max_y, max_x);
    max_x = 50;
    limx2 = max_x - 4;
    limy2 = max_y - 3;

    // Posición inicial de la nave
    x_nave = limx1 + 1;
}

// Función para mostrar el menú de selección de modo de juego
void select_game_mode() {
    clear();
    mvprintw(max_y / 2 - 2, max_x / 2 - 10, "Select Game Mode:");
    mvprintw(max_y / 2, max_x / 2 - 10, "1 - Easy Mode (8 Aliens per round)");
    mvprintw(max_y / 2 + 1, max_x / 2 - 10, "2 - Hard Mode (10 Aliens per round)");
    refresh();

    int ch;
    while (true) {
        ch = getch();
        if (ch == '1') {
            mode = 0;
            aliens_per_round = 8;
            break;
        } else if (ch == '2') {
            mode = 1;
            aliens_per_round = 10;
            break;
        }
    }

    // Inicializar los aliens según el modo seleccionado
    pthread_mutex_lock(&aliens_mutex);
    for (int i = 0; i < aliens_per_round; ++i) {
        aliens.push_back({(limx1 + 8) + i * 5, limy1 + ((i % 2 == 0) ? 4 : 2), true});
    }
    pthread_mutex_unlock(&aliens_mutex);
}

// Función para reiniciar la ronda cuando todos los aliens son eliminados
void next_round() {
    current_round++;
    if (current_round > total_rounds) {
        game_over = true;
        return;
    }

    pthread_mutex_lock(&aliens_mutex);
    aliens.clear();  // Limpiar los aliens anteriores
    for (int i = 0; i < aliens_per_round; ++i) {
        aliens.push_back({(limx1 + 8) + i * 5, limy1 + ((i % 2 == 0) ? 4 : 2), true});
    }
    pthread_mutex_unlock(&aliens_mutex);
}

// Función para dibujar los bordes del juego
void draw_borders() {
    mvvline(limy1 + 1, limx1 - 1, '|', limy2 - limy1 - 1);
    mvvline(limy1 + 1, limx2 + 3, '|', limy2 - limy1 - 1);
    mvhline(limy1, limx1 - 1, '-', limx2 - limx1 + 5);
    mvhline(limy2, limx1 - 1, '-', limx2 - limx1 + 5);
}

// Función para dibujar la nave
void draw_ship() {
    for (int i = 0; i < 2; ++i) {
        mvprintw(limy2 - 2 + i, x_nave, nave[i]);
    }
}

// Función para dibujar los proyectiles de la nave
void draw_projectiles_player() {
    pthread_mutex_lock(&projectilePlayer_mutex);
    for (const auto& proj : projectilePlayers) {
        mvprintw(proj.y, proj.x, "|");
    }
    pthread_mutex_unlock(&projectilePlayer_mutex);
}

// Función para dibujar los proyectiles de los aliens
void draw_projectiles_alien() {
    pthread_mutex_lock(&projectileAlien_mutex);
    for (const auto& proj : projectileAliens) {
        mvprintw(proj.y, proj.x, "v");
    }
    pthread_mutex_unlock(&projectileAlien_mutex);
}

// Función para manejar el disparo de proyectiles de la nave
void handle_shooting_player() {
    pthread_mutex_lock(&projectilePlayer_mutex);
    if (fire_projectilePlayer) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_shot_time).count();
        
        if (duration >= 500) {
            projectilePlayers.push_back({x_nave + 1, limy2 - 3});  // Disparar desde la nave
            fire_projectilePlayer = false;  // Resetear la bandera
            last_shot_time = now;
        }
    }
    pthread_mutex_unlock(&projectilePlayer_mutex);
}

// Función para manejar el disparo de proyectiles de los aliens
void handle_shooting_alien() {
    pthread_mutex_lock(&aliens_mutex);
    pthread_mutex_lock(&projectileAlien_mutex);

    // Cada alien tiene una pequeña probabilidad de disparar
    for (const auto& alien : aliens) {
        if (rand() % 100 < 5) {  // 5% de probabilidad de disparo
            projectileAliens.push_back({alien.x + 1, alien.y + 1});  // Disparo alienígena desde la posición del alien
        }
    }

    pthread_mutex_unlock(&projectileAlien_mutex);
    pthread_mutex_unlock(&aliens_mutex);
}

// Función para dibujar los alienígenas
void draw_aliens() {
    pthread_mutex_lock(&aliens_mutex);
    for (const auto& alienObject : aliens) {
        mvprintw(alienObject.y, alienObject.x, alien);  
    }
    pthread_mutex_unlock(&aliens_mutex);
}

// Función para capturar la entrada del jugador y mover la nave
void handle_input() {
    int ch = getch();

    if (ch == KEY_LEFT && x_nave > limx1) {
        x_nave--;  // Mover la nave a la izquierda
    } else if (ch == KEY_RIGHT && x_nave < limx2) {
        x_nave++;  // Mover la nave a la derecha
    } else if (ch == ' ') {
        pthread_mutex_lock(&projectilePlayer_mutex);
        fire_projectilePlayer = true;  // Indicar que se disparó un proyectil
        pthread_mutex_unlock(&projectilePlayer_mutex);
    } else if (ch == 'q') {
        // Si presiona 'q', terminar el juego
        endwin();
        pthread_mutex_destroy(&projectilePlayer_mutex);
        pthread_mutex_destroy(&projectileAlien_mutex);
        exit(0);
    }
}

// Función para detectar colisiones entre los proyectiles de la nave y los aliens
void detect_collisions() {
    pthread_mutex_lock(&projectilePlayer_mutex);
    pthread_mutex_lock(&aliens_mutex);

    for (auto proj = projectilePlayers.begin(); proj != projectilePlayers.end();) {
        bool hit = false;
        for (auto alien = aliens.begin(); alien != aliens.end();) {
            if (proj->x >= alien->x && proj->x <= alien->x + 2 && proj->y == alien->y) {
                // Si hay colisión
                proj = projectilePlayers.erase(proj);
                alien = aliens.erase(alien);
                score += 100;
                hit = true;
                break;
            } else {
                ++alien;
            }
        }
        if (!hit) {
            ++proj;
        }
    }

    pthread_mutex_unlock(&aliens_mutex);
    pthread_mutex_unlock(&projectilePlayer_mutex);
}

// Función para manejar el movimiento de los aliens
void handle_alien_movement() {
    pthread_mutex_lock(&aliens_mutex);

    for (auto& alienObject : aliens) {
        if (alienObject.direction) {
            alienObject.x++;
        } else {
            alienObject.x--;
        }

        if (alienObject.x >= limx2 || alienObject.x <= limx1) {
            alienObject.direction = !alienObject.direction;
            alienObject.y++;
        }
    }

    pthread_mutex_unlock(&aliens_mutex);
}

// Función para mover los proyectiles
void move_projectiles() {
    // Mover proyectiles del jugador
    pthread_mutex_lock(&projectilePlayer_mutex);
    for (auto proj = projectilePlayers.begin(); proj != projectilePlayers.end();) {
        proj->y--;
        if (proj->y <= limy1) {
            proj = projectilePlayers.erase(proj);  // Eliminar proyectiles que salen de la pantalla
        } else {
            ++proj;
        }
    }
    pthread_mutex_unlock(&projectilePlayer_mutex);

    // Mover proyectiles de los aliens
    pthread_mutex_lock(&projectileAlien_mutex);
    for (auto proj = projectileAliens.begin(); proj != projectileAliens.end();) {
        proj->y++;
        if (proj->y >= limy2) {
            proj = projectileAliens.erase(proj);  // Eliminar proyectiles que salen de la pantalla
        } else {
            ++proj;
        }
    }
    pthread_mutex_unlock(&projectileAlien_mutex);
}

// Función principal del juego
void* game_loop(void*) {
    while (!game_over) {
        clear();

        draw_borders();          // Dibujar los bordes del juego
        draw_ship();             // Dibujar la nave
        draw_aliens();           // Dibujar los alienígenas
        draw_projectiles_player();  // Dibujar los proyectiles de la nave
        draw_projectiles_alien();   // Dibujar los proyectiles de los aliens

        handle_input();          // Manejar la entrada del jugador
        handle_shooting_player();  // Manejar disparos de la nave
        handle_shooting_alien();   // Manejar disparos de los aliens
        detect_collisions();     // Detectar colisiones entre proyectiles de la nave y los aliens
        handle_alien_movement(); // Manejar movimiento de los aliens
        move_projectiles();      // Mover los proyectiles

        // Mostrar el puntaje y las vidas restantes
        mvprintw(limy2 + 1, limx1, "Score: %d", score);
        mvprintw(limy2 + 1, limx2 - 10, "Lives: %d", lives);
        mvprintw(limy2 + 2, limx1, "Round: %d", current_round);

        // Comprobar si se han eliminado todos los aliens
        pthread_mutex_lock(&aliens_mutex);
        if (aliens.empty()) {
            next_round();  // Avanzar a la siguiente ronda
        }
        pthread_mutex_unlock(&aliens_mutex);

        refresh();
        usleep(50000);  // Esperar 50ms entre cada iteración del loop
    }

    endwin();
    return NULL;
}

int main() {
    srand(time(0));  // Inicializar semilla para números aleatorios

    // Inicializar ncurses y configurar la pantalla
    init_screen();

    // Mostrar el menú de selección de modo de juego
    select_game_mode();

    // Inicializar los mutex
    pthread_mutex_init(&projectilePlayer_mutex, NULL);
    pthread_mutex_init(&projectileAlien_mutex, NULL);
    pthread_mutex_init(&aliens_mutex, NULL);

    // Crear el thread del loop del juego
    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);

    // Esperar a que termine el thread del juego
    pthread_join(game_thread, NULL);

    // Destruir los mutex y finalizar ncurses
    pthread_mutex_destroy(&projectilePlayer_mutex);
    pthread_mutex_destroy(&projectileAlien_mutex);
    pthread_mutex_destroy(&aliens_mutex);
    endwin();

    return 0;
}
