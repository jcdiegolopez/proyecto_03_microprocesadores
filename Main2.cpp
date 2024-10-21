#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <random>

// Estructura para manejar los proyectiles
struct projectilePlayer {
    int x, y; // Posición del proyectil
};

// Estructura para movimiento de los aliens
struct Alien {
    int x, y;
    bool direction;  // true: derecha, false: izquierda
};

// Variables globales compartidas
std::vector<Alien> aliens;                        // Lista de aliens
std::vector<projectilePlayer> projectilePlayers;  // Lista de proyectiles
pthread_mutex_t projectilePlayer_mutex;           // Mutex para proteger la lista de proyectiles
pthread_mutex_t aliens_mutex;                     // Mutex para proteger la lista de aliens
bool fire_projectilePlayer = false;               // Bandera para indicar cuándo disparar un proyectil
int score = 0;                                    // Variable para almacenar el puntaje

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

    // Inicializar aliens
    pthread_mutex_lock(&aliens_mutex);

    // Crear 5 alienígenas 
    for (int i = 0; i < 5; ++i) {
        aliens.push_back({(limx1 + 8) + i * 8, limy1 + ((i % 2 == 0) ? 4 : 2), true});
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

// Función para dibujar los proyectiles
void draw_projectiles() {
    pthread_mutex_lock(&projectilePlayer_mutex);
    for (const auto& proj : projectilePlayers) {
        mvprintw(proj.y, proj.x, "|");
    }
    pthread_mutex_unlock(&projectilePlayer_mutex);
}

// Función para manejar el disparo de proyectiles
void handle_shooting() {
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
        exit(0);
    }
}

// Función para detectar colisiones y eliminar aliens
void detect_collisions() {
    pthread_mutex_lock(&aliens_mutex);
    pthread_mutex_lock(&projectilePlayer_mutex);

    for (auto proj_it = projectilePlayers.begin(); proj_it != projectilePlayers.end(); ) {
        bool hit = false;
        for (auto alien_it = aliens.begin(); alien_it != aliens.end(); ) {
            // Detectar colisión entre proyectil y alien
            if (proj_it->x == alien_it->x && proj_it->y == alien_it->y) {
                alien_it = aliens.erase(alien_it);  // Eliminar alien
                hit = true;
                score += 10;  // Sumar 10 puntos al puntaje
                break;
            } else {
                ++alien_it;
            }
        }
        if (hit) {
            proj_it = projectilePlayers.erase(proj_it);  // Eliminar proyectil si hubo colisión
        } else {
            ++proj_it;
        }
    }

    pthread_mutex_unlock(&projectilePlayer_mutex);
    pthread_mutex_unlock(&aliens_mutex);
}

// Función que se ejecuta en el hilo de los proyectiles
void* projectile_thread(void* arg) {
    while (true) {
        pthread_mutex_lock(&projectilePlayer_mutex);

        // Mover proyectiles hacia arriba
        for (auto it = projectilePlayers.begin(); it != projectilePlayers.end();) {
            it->y--;
            if (it->y < limy1 + 1) {
                it = projectilePlayers.erase(it);
            } else {
                ++it;
            }
        }

        pthread_mutex_unlock(&projectilePlayer_mutex);
        usleep(60000);  // Controlar la velocidad de los proyectiles
    }
    return NULL;
}

// Generador de números aleatorios
int generate_random_int(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

// Función que se ejecuta en el hilo de los alienígenas
void* alien_thread(void* arg) {
    stepLimit = generate_random_int(0, limx2 * 2);  // Límite inicial de pasos
    counter = 0;  // Contador de pasos

    while (true) {
        pthread_mutex_lock(&aliens_mutex);

        if (counter > stepLimit) {
            for (auto& alien : aliens) {
                alien.direction = !alien.direction;  // Cambiar la dirección de todos los alienígenas
                alien.y++;  // Todos bajan una línea
            }
            counter = 0;
            stepLimit = generate_random_int(0, limx2 * 2);
        }

        for (auto& alien : aliens) {
            if (alien.direction) {
                alien.x++;  // Mover hacia la derecha
                if (alien.x >= limx2) {
                    alien.direction = false;  // Cambiar dirección al llegar al límite derecho
                    alien.y++;
                }
            } else {
                alien.x--;  // Mover hacia la izquierda
                if (alien.x <= limx1) {
                    alien.direction = true;  // Cambiar dirección al llegar al límite izquierdo
                    alien.y++;
                }
            }
        }

        counter++;
        pthread_mutex_unlock(&aliens_mutex);
        usleep(120000);  // Controlar la velocidad de los alienígenas
    }

    return NULL;
}

// Función para mostrar el puntaje
void draw_score() {
    mvprintw(1, 2, "Score: %d", score);
}

int main() {
    init_screen();  // Inicializar ncurses y pantalla
    pthread_mutex_init(&projectilePlayer_mutex, NULL);
    pthread_mutex_init(&aliens_mutex, NULL);

    // Crear el hilo para manejar los proyectiles
    pthread_t projectile_tid;
    pthread_create(&projectile_tid, NULL, projectile_thread, NULL);

    // Crear el hilo para manejar los alienígenas
    pthread_t alien_tid;
    pthread_create(&alien_tid, NULL, alien_thread, NULL);

    // Bucle principal del juego
    while (true) {
        clear();  // Limpiar la pantalla

        draw_borders();      // Dibujar los bordes del juego
        draw_ship();         // Dibujar la nave
        draw_aliens();       // Dibujar los aliens
        handle_input();      // Manejar la entrada del usuario
        handle_shooting();   // Manejar los disparos
        draw_projectiles();  // Dibujar los proyectiles
        detect_collisions(); // Detectar colisiones
        draw_score();        // Mostrar el puntaje

        refresh();
        usleep(30000);  // Controlar la velocidad del juego
    }

    return 0;
}
