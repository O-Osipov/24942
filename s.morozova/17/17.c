#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 40

// Управляющие символы
#define ERASE 0x7F  // Backspace (127)
#define KILL 0x15   // Ctrl-U (21)
#define CTRL_W 0x17 // Ctrl-W (23)
#define CTRL_D 0x04 // Ctrl-D (4)

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
    write(STDOUT_FILENO, "\x07", 1);
}

// Удаляет последнее слово в строке
void erase_last_word(char *line, int *pos) {
    if (*pos == 0) {
        sound_bell();
        return;
    }

    int end = *pos;
    
    // Пропускаем пробелы в конце
    while (*pos > 0 && isspace(line[*pos - 1])) {
        (*pos)--;
    }
    
    // Удаляем слово
    while (*pos > 0 && !isspace(line[*pos - 1])) {
        (*pos)--;
    }

    // Очищаем экран и перерисовываем строку
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

    printf("Введите текст (Ctrl-D для выхода):\n");

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        // Проверка на Ctrl-D в начале строки
        if (c == CTRL_D && pos == 0) {
            printf("\n");
            break;
        }

        // Обработка специальных символов
        if (c == ERASE) { // Backspace
            if (pos > 0) {
                pos--;
                col--;
                printf("\b \b");
                fflush(stdout);
            } else {
                sound_bell();
            }
        }
        else if (c == KILL) { // Удалить всю строку
            while (pos > 0) {
                pos--;
                col--;
                printf("\b \b");
            }
            fflush(stdout);
        }
        else if (c == CTRL_W) { // Удалить последнее слово
            erase_last_word(line, &pos);
            col = pos;
        }
        else if (c >= 32 && c <= 126) { // Печатаемые символы
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
        else { // Непечатаемые символы
            sound_bell();
        }

        line[pos] = '\0';
    }

    return 0;
}