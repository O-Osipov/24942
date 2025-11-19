#include <stdio.h> 
//isprint - возращает true если это печатаемый ASCII символ
#include <unistd.h> 
#include <termios.h>
#include <ctype.h>

#define MAXLEN 40 

int main(void) {
    struct termios oldt, t;
    //считаем текущие атрибуты терминала 
    tcgetattr(0, &oldt);
    t = oldt; 

    //выключаем канонический режим и эхо
    t.c_lflag &= ~(ICANON | ECHO);

    //читать по одному байту
    t.c_cc[VMIN] = 1; 
    t.c_cc[VTIME] = 0; 

    //применяем новые настройки 
    tcsetattr(0, TCSANOW, &t);

    char line[MAXLEN + 1];
    int len = 0; //текущая длина строки 
    int col = 0; //номер колонки в строке  

    while (1) {
        char c; 
        /*
        Raw mode — это режим работы терминала, в котором ядро перестаёт обрабатывать ввод,
        а программа получает символы напрямую, один за другим, без какой-либо интерпретации, 
        задержки и редактирования.
        */
        read(0, &c, 1); //читаем один символ (в raw mode)


        //CTRL-D - завершение 
        if (c == 4 && len == 0){ //ctrlD и пустая строка
            write(1, "\n", 1);
            break;
        }

        //BACKSPACE /ERASE
        if (c == 127 || c == 8) { //DEL || Backspace
            if (len > 0 ){
                len--;
                col--;
                write(1, "\b \b", 3); //удаление символа на терминале 
            }
            continue;
        }
                //KILL (Ctrl-U) 
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
            //удалим пробелы справа
            while (len > 0 && isspace((unsigned char)line[len-1])){
                write(1, "\b \b", 3);
                len--; 
                col--;
            }
            //удаляем буквы слова
            while (len > 0 && !isspace((unsigned char)line[len-1])){
                write(1, "\b \b", 3);
                len--; 
                col--;
            }
            continue; 
        }
        //печатаемый символ
        if (isprint((unsigned char)c)){
            //перенос строки при достижении MAXLEN 
            if (col == MAXLEN){
                write(1, "\n", 1);
                len = 0;
                col = 0; 
                line[0] = '\0';    
            }
            //добавляем символ в буфер 
            line[len++] = c; 
            col++; 

            //НЕМЕДЛЕННО выводим
            write (1, &c, 1);
            continue;
        }
        //все остальные -> звуковой сигнал 
        write(1, "\a", 1);
    }
    //восстанавливаем режим терминала
    tcsetattr(0, TCSAFLUSH, &oldt);

    return 0; 
}