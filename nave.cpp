#include<stdio.h>
#include<windows.h>
#include<conio.h> //para detectar pulsaciones de teclas.

//Funcion para movimiento
void gotoxy(int x, int y){
//Coordenadas
    HANDLE hCON;
    hCON = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD dwPos;
    dwPos.X = x;
    dwPos.Y = y;    //El eje Y es positivo hacia abajo
    SetConsoleCursorPosition(hCON, dwPos);
}

int main(){
    int x = 10, y = 10;
    gotoxy(x,y);
    printf("*");

    bool game_over = false;
    while(!game_over){
        gotoxy(x,y);
        printf("*");

        if(kbhit()){ //funcion de la libreria conio
        char tecla = getch(); //capturar que tecla se presiona
        gotoxy(x,y); printf(" ");
        if (tecla == 'a'){ //movimiento a la izquierda
            x--;
        }
        if (tecla == 'd'){ //movimiento a la derecha
            x++;
        }
        if (tecla == 'w'){ //movimiento hacia arriba
            y--;
        }
        if (tecla == 's'){ //movimiento hacia abajo
            y++;
        }

        }

        Sleep(30);
    }

    return 0;
}