#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <chrono>

// Estructura para manejar los proyectiles
struct projectilePlayer {
    int x, y; // Posición del proyectil
};

// Variables globales compartidas
std::vector<projectilePlayer> projectilePlayers;  // Lista de proyectiles
pthread_mutex_t projectilePlayer_mutex;           // Mutex para proteger la lista de proyectiles
bool fire_projectilePlayer = false;               // Bandera para indicar cuándo disparar un proyectil

// Dimensiones de la pantalla
int max_y, max_x;
int limx1 = 1, limx2, limy1 = 2, limy2;

// Estructura de la nave
const char* nave[] = {" A", "/_\\"};
int x_nave;

// Tiempo del último disparo
auto last_shot_time = std::chrono::high_resolution_clock::now();

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
        //Checkear hace cuanto se hizo el último disparo
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_shot_time).count();
        

        if(duration >= 500){
            projectilePlayers.push_back({x_nave + 1, limy2 - 3});  // Disparar desde la nave
            fire_projectilePlayer = false;  // Resetear la bandera
            last_shot_time = now;
        }
        
    }
    pthread_mutex_unlock(&projectilePlayer_mutex);
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

// Función que se ejecuta en el hilo de los proyectiles
void* projectile_thread(void* arg) {
    while (true) {
        pthread_mutex_lock(&projectilePlayer_mutex);

        // Mover proyectiles hacia arriba
        for (auto it = projectilePlayers.begin(); it != projectilePlayers.end();) {
            it->y--;
            // Eliminar proyectiles que salgan de la pantalla
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

int main() {
    init_screen();  // Inicializar ncurses y pantalla
    pthread_mutex_init(&projectilePlayer_mutex, NULL);

    // Crear el hilo para manejar los proyectiles
    pthread_t projectile_tid;
    pthread_create(&projectile_tid, NULL, projectile_thread, NULL);

    // Bucle principal del juego
    while (true) {
        clear();  // Limpiar la pantalla

        draw_borders();      // Dibujar los bordes del juego
        draw_ship();         // Dibujar la nave
        handle_input();      // Manejar la entrada del usuario
        handle_shooting();   // Manejar los disparos
        draw_projectiles();  // Dibujar los proyectiles

        refresh();
        usleep(30000);  // Controlar la velocidad del juego
    }

    return 0;
}
