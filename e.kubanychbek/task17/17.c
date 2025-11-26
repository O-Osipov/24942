#include <stdio.h> 
#include <unistd.h> 
#include <termios.h>
#include <ctype.h>

#define MAXLEN 40 

int main(void) {
    struct termios oldt, t;
    tcgetattr(0, &oldt);
    t = oldt; 

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1; 
    t.c_cc[VTIME] = 0; 
    tcsetattr(0, TCSANOW, &t);

    char line[MAXLEN + 1];
    int len = 0;  
    int col = 0; 
    while (1) {
        char c; 
        read(0, &c, 1); 

        //CTRL-D - завершение 
        if (c == 4 && len == 0){ 
            write(1, "\n", 1);
            break;
        }

        //BACKSPACE /ERASE
        if (c == 127 || c == 8) { 
            if (len > 0 ){
                len--;
                col--;
                write(1, "\b \b", 3); //удаление символа на терминале 
            }
            continue;
        }
        //KILL (Ctrl-U)  стираем все со строки 
        if (c == 21) { 
            while (len > 0) {
                write(1, "\b \b", 3);
                len--;
                col--;
            }
            continue;
        }
        //CTRL-W удаляем последнее слово 
        if (c == 23){
            while (len > 0 && isspace((unsigned char)line[len-1])){
                write(1, "\b \b", 3);
                len--; 
                col--;
            }
            while (len > 0 && !isspace((unsigned char)line[len-1])){
                write(1, "\b \b", 3);
                len--; 
                col--;
            }
            continue; 
        }
        if (isprint((unsigned char)c)){
            if (col == MAXLEN){
                write(1, "\n", 1);
                len = 0;
                col = 0; 
                line[0] = '\0';    
            } 
            line[len++] = c; 
            col++; 
            write (1, &c, 1);
            continue;
        }
        write(1, "\a", 1);
    }
    tcsetattr(0, TCSAFLUSH, &oldt);
    return 0; 
}