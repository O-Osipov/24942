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
    printf("\a"); // Простой способ издать звуковой сигнал
    fflush(stdout);
}

void erase_last_word(char *line, int *pos) {
    if (*pos == 0) {
        sound_bell();
        return;
    }

    // Пропускаем пробелы в конце
    while (*pos > 0 && isspace(line[*pos - 1])) {
        (*pos)--;
    }
    
    // Удаляем слово
    while (*pos > 0 && !isspace(line[*pos - 1])) {
        (*pos)--;
    }

    // Перерисовываем строку
    printf("\r\033[K"); // Возврат и очистка строки
    if (*pos > 0) {
        write(STDOUT_FILENO, line, *pos);
    }
    fflush(stdout);
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
        if (c == 4 && pos == 0) { // 4 = Ctrl-D
            printf("\n");
            break;
        }

        // Backspace (127 или 8)
        if (c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                col--;
                printf("\b \b");
                fflush(stdout);
            } else {
                sound_bell();
            }
        }
        // Ctrl-U - удалить всю строку
        else if (c == 21) {
            while (pos > 0) {
                pos--;
                col--;
                printf("\b \b");
            }
            fflush(stdout);
        }
        // Ctrl-W - удалить последнее слово
        else if (c == 23) {
            erase_last_word(line, &pos);
            col = pos;
        }
        // Печатаемые символы
        else if (c >= 32 && c <= 126) {
            if (pos < MAX_LINE) {
                line[pos++] = c;
                col++;
                
                // Перенос строки при достижении 40 символов
                if (col >= MAX_LINE) {
                    printf("\n");
                    col = 0;
                }
                
                write(STDOUT_FILENO, &c, 1);
                fflush(stdout);
            } else {
                sound_bell();
            }
        }
        // Непечатаемые символы
        else {
            sound_bell();
        }

        line[pos] = '\0';
    }

    return 0;
}