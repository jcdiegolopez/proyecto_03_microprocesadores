#include <ncursesw/ncurses.h>
#include <unistd.h>  // Para usar usleep()

int main() {
    // Inicializa ncurses
    initscr();            // Inicializa la pantalla
    noecho();             // Evita que las teclas presionadas se muestren en la pantalla
    curs_set(FALSE);      // Oculta el cursor
    keypad(stdscr, TRUE); // Permite capturar teclas especiales como las flechas
    timeout(100);         // No bloquea la entrada, espera 100ms por una tecla

    // Obtener las dimensiones de la pantalla
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);  // Obtén las dimensiones de la pantalla
    max_x = 50;

    // Ajustamos los límites
    int limx1 = 1;           // Borde izquierdo (dejamos un espacio para el borde)
    int limx2 = max_x - 4;   // Borde derecho (considerando 3 caracteres de la nave y 1 más para el borde)
    int limy1 = 2;           // Borde superior (dejamos 2 líneas de espacio)
    int limy2 = max_y - 3;   // Borde inferior (dejamos espacio para la nave y el borde)

    // Definición de la nave como un array de strings
    const char* nave[] = {" A", "/_\\"};
    int x_nave = limx1 + 1;  // Posición inicial de la nave en el eje X (horizontal)

    // Bucle principal del juego
    while (1) {
        clear();  // Limpiar la pantalla en cada frame

        // Dibuja los bordes de la pantalla
        mvvline(limy1 + 1, limx1 - 1, '|', limy2 - limy1 - 1);         // Línea vertical en el borde izquierdo
        mvvline(limy1 + 1, limx2 + 3, '|', limy2 - limy1 - 1);         // Línea vertical en el borde derecho
        mvhline(limy1, limx1 - 1, '-', limx2 - limx1 + 5);         // Línea horizontal superior
        mvhline(limy2, limx1 - 1, '-', limx2 - limx1 + 5);         // Línea horizontal inferior

        // Dibuja la nave en la posición actual
        for (int i = 0; i < 2; ++i) {
            mvprintw(limy2 - 2 + i, x_nave, nave[i]);
        }

        // Captura la entrada del usuario
        int ch = getch();

        // Mover la nave a la izquierda o derecha dependiendo de la tecla presionada
        if (ch == KEY_LEFT) {
            if (x_nave > limx1) x_nave--;  // Mover izquierda si no está en el borde izquierdo
        } else if (ch == KEY_RIGHT) {
            if (x_nave < limx2) x_nave++;  // Mover derecha si no está en el borde derecho
        } else if (ch == 'q') {
            break;  // Salir del juego si se presiona 'q'
        }

        // Refrescar la pantalla para mostrar los cambios
        refresh();

        // Controlar la velocidad del juego
        usleep(30000);  // Pausa de 30 milisegundos entre frames
    }

    endwin();  // Finaliza ncurses
    return 0;
}
