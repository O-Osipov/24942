#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE_LENGTH 40
#define ERASE_CHAR 0x7F  // ASCII DEL (обычно Backspace)
#define KILL_CHAR 0x15   // ASCII NAK (Ctrl-U)
#define CTRL_W 0x17      // ASCII ETB (Ctrl-W)
#define CTRL_D 0x04      // ASCII EOT (Ctrl-D)
#define CTRL_G 0x07      // ASCII BEL (звуковой сигнал)

// Структура для хранения состояния строки
typedef struct {
    char buffer[MAX_LINE_LENGTH + 1];
    int length;
    int cursor_pos;
} LineBuffer;

// Инициализация буфера строки
void init_line_buffer(LineBuffer *lb) {
    lb->length = 0;
    lb->cursor_pos = 0;
    memset(lb->buffer, 0, sizeof(lb->buffer));
}

// Добавление символа в буфер
int add_char(LineBuffer *lb, char c) {
    if (lb->length >= MAX_LINE_LENGTH) {
        return -1; // Достигнут предел длины
    }
    
    // Сдвигаем символы справа от курсора
    for (int i = lb->length; i > lb->cursor_pos; i--) {
        lb->buffer[i] = lb->buffer[i - 1];
    }
    
    lb->buffer[lb->cursor_pos] = c;
    lb->length++;
    lb->cursor_pos++;
    lb->buffer[lb->length] = '\0';
    
    return 0;
}

// Удаление символа перед курсором
void erase_char(LineBuffer *lb) {
    if (lb->cursor_pos > 0 && lb->length > 0) {
        // Сдвигаем символы слева от курсора
        for (int i = lb->cursor_pos - 1; i < lb->length - 1; i++) {
            lb->buffer[i] = lb->buffer[i + 1];
        }
        lb->length--;
        lb->cursor_pos--;
        lb->buffer[lb->length] = '\0';
    }
}

// Удаление всей строки
void kill_line(LineBuffer *lb) {
    lb->length = 0;
    lb->cursor_pos = 0;
    memset(lb->buffer, 0, sizeof(lb->buffer));
}

// Удаление последнего слова
void erase_word(LineBuffer *lb) {
    if (lb->cursor_pos == 0) return;
    
    int new_pos = lb->cursor_pos;
    
    // Пропускаем пробелы
    while (new_pos > 0 && isspace(lb->buffer[new_pos - 1])) {
        new_pos--;
    }
    
    // Удаляем символы слова
    while (new_pos > 0 && !isspace(lb->buffer[new_pos - 1])) {
        new_pos--;
    }
    
    // Удаляем символы между new_pos и cursor_pos
    int chars_to_remove = lb->cursor_pos - new_pos;
    if (chars_to_remove > 0) {
        for (int i = new_pos; i < lb->length - chars_to_remove; i++) {
            lb->buffer[i] = lb->buffer[i + chars_to_remove];
        }
        lb->length -= chars_to_remove;
        lb->cursor_pos = new_pos;
        lb->buffer[lb->length] = '\0';
    }
}

// Перерисовка строки на терминале
void redraw_line(LineBuffer *lb) {
    // Очищаем строку
    printf("\r\033[K"); // Возврат каретки и очистка строки
    
    // Выводим текущее содержимое
    if (lb->length > 0) {
        printf("%s", lb->buffer);
    }
    
    // Позиционируем курсор
    if (lb->cursor_pos < lb->length) {
        printf("\033[%dD", lb->length - lb->cursor_pos);
    }
    fflush(stdout);
}

// Проверка, является ли символ печатаемым
int is_printable(char c) {
    return (c >= 32 && c <= 126); // Печатаемые ASCII символы
}

int main() {
    struct termios old_termios, new_termios;
    LineBuffer line_buffer;
    
    // Сохраняем текущие настройки терминала
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    
    // Отключаем канонический режим и эхо
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    
    // Применяем новые настройки
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    
    init_line_buffer(&line_buffer);
    
    printf("Редактор строки (макс. %d символов)\n", MAX_LINE_LENGTH);
    printf("Управление: Backspace - удалить символ, Ctrl-U - удалить строку, Ctrl-W - удалить слово, Ctrl-D - выход\n");
    
    int running = 1;
    while (running) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == CTRL_D) {
                // Ctrl-D: выход если курсор в начале строки
                if (line_buffer.cursor_pos == 0) {
                    running = 0;
                } else {
                    printf("%c", CTRL_G); // Звуковой сигнал
                }
            } else if (c == ERASE_CHAR) {
                // Удаление последнего символа
                erase_char(&line_buffer);
                redraw_line(&line_buffer);
            } else if (c == KILL_CHAR) {
                // Удаление всей строки
                kill_line(&line_buffer);
                redraw_line(&line_buffer);
            } else if (c == CTRL_W) {
                // Удаление последнего слова
                erase_word(&line_buffer);
                redraw_line(&line_buffer);
            } else if (is_printable(c)) {
                // Добавление печатаемого символа
                if (add_char(&line_buffer, c) == 0) {
                    redraw_line(&line_buffer);
                } else {
                    printf("%c", CTRL_G); // Звуковой сигнал при переполнении
                }
            } else {
                // Непечатаемый символ - звуковой сигнал
                printf("%c", CTRL_G);
            }
        }
    }
    
    // Восстанавливаем настройки терминала
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    
    printf("\nПрограмма завершена.\n");
    return 0;
}