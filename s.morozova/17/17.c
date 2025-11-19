#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 40

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void sound_bell() {
    write(STDOUT_FILENO, "\a", 1);
}

void redraw_line(const char *line, int pos) {
    printf("\r\033[K"); // курсор в начало, очистить строку
    if (pos > 0) {
        write(STDOUT_FILENO, line, pos);
    }
    fflush(stdout);
}

void erase_last_word(char *line, int *pos) {
    if (*pos == 0) {
        sound_bell();
        return;
    }

    int new_pos = *pos;
    
    // Пропускаем пробелы в конце
    while (new_pos > 0 && isspace(line[new_pos - 1])) {
        new_pos--;
    }
    
    // Удаляем слово
    while (new_pos > 0 && !isspace(line[new_pos - 1])) {
        new_pos--;
    }

    *pos = new_pos;
    redraw_line(line, *pos);
}

int main() {
    enable_raw_mode();

    char line[MAX_LINE + 1] = {0};
    int pos = 0;
    int col = 0;

    printf("Введите текст (Ctrl-D для выхода): ");
    fflush(stdout);

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        // Ctrl-D в начале строки - выход
        if (c == 4 && pos == 0) {
            printf("\n");
            break;
        }

        switch (c) {
            case 127: // Backspace
            case 8:   // Backspace (альтернативный код)
                if (pos > 0) {
                    pos--;
                    col = pos;
                    redraw_line(line, pos);
                } else {
                    sound_bell();
                }
                break;
                
            case 21: // Ctrl-U - удалить всю строку
                pos = 0;
                col = 0;
                redraw_line(line, pos);
                break;
                
            case 23: // Ctrl-W - удалить последнее слово
                erase_last_word(line, &pos);
                col = pos;
                break;
                
            default:
                if (c >= 32 && c <= 126) { // Печатаемые символы
                    if (pos < MAX_LINE) {
                        line[pos++] = c;
                        col = pos;
                        
                        // Автоматический перенос
                        if (col >= MAX_LINE) {
                            printf("\n");
                            col = 0;
                        }
                        
                        redraw_line(line, pos);
                    } else {
                        sound_bell();
                    }
                } else {
                    sound_bell();
                }
                break;
        }
        
        line[pos] = '\0';
    }

    return 0;
}