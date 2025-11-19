#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 40
#define BELL '\x07'

// Глобальные переменные для хранения исходных настроек терминала
static struct termios orig_termios;

// Функция для восстановления исходных настроек терминала
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Функция для настройки терминала в неканонический режим без эха
void set_terminal_raw(void) {
    struct termios raw;
    
    // Получаем текущие настройки терминала
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    
    // Регистрируем функцию восстановления при выходе
    atexit(restore_terminal);
    
    raw = orig_termios;
    
    // Отключаем канонический режим, эхо и обработку специальных символов
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    
    // Устанавливаем минимальное количество символов для чтения и время ожидания
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    // Применяем новые настройки
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

// Функция для проверки, является ли символ печатаемым
int is_printable_char(char c) {
    return (c >= 32 && c <= 126); // ASCII печатаемые символы
}

// Функция для проверки, является ли символ разделителем слова
int is_word_delimiter(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void sound_bell(void) {
    printf("%c", BELL);
    fflush(stdout);
}
// Функция для удаления последнего слова из строки
void delete_last_word(char *line, int *pos, int *column) {
    if (*pos == 0) return; // Строка пуста
    
    int i = *pos - 1;
    int chars_to_delete = 0;
    
    // Пропускаем завершающие пробелы
    while (i >= 0 && is_word_delimiter(line[i])) {
        i--;
        chars_to_delete++;
    }
    
    // Находим начало последнего слова
    while (i >= 0 && !is_word_delimiter(line[i])) {
        i--;
        chars_to_delete++;
    }
    
    // Удаляем символы с экрана
    for (int j = 0; j < chars_to_delete; j++) {
        printf("\b \b");
    }
    
    // Обновляем позиции
    *pos -= chars_to_delete;
    *column -= chars_to_delete;
}

// Функция для проверки и обработки перехода на новую строку
void check_line_wrap(char *line, int *pos, int *column) {
    if (*column >= MAX_LINE_LENGTH) {
        // Находим начало текущего слова
        int word_start = *pos - 1;
        while (word_start >= 0 && !is_word_delimiter(line[word_start])) {
            word_start--;
        }
        word_start++; // Переходим к первому символу слова
        
        // Если слово слишком длинное, просто переносим его
        if (word_start == 0 && *pos > MAX_LINE_LENGTH) {
            printf("\n");
            *column = *pos; // Сбрасываем счетчик колонки
        } else if (word_start > 0) {
            // Переносим слово на новую строку
            printf("\n");
            int word_length = *pos - word_start;
            
            // Выводим слово на новой строке
            for (int i = word_start; i < *pos; i++) {
                printf("%c", line[i]);
            }
            
            *column = word_length;
        }
    }
}

int main(void) {
    char line[1024]; // Буфер для ввода
    int pos = 0;     // Текущая позиция в строке
    int column = 0;  // Текущая колонка на экране
    
    // Настраиваем терминал
    set_terminal_raw();
    
    printf("Редактор строки (ERASE=Backspace, KILL=Ctrl+U, DELETE WORD=Ctrl+W, EXIT=Ctrl+D в начале строки)\n");
    printf("Максимальная длина строки: %d символов\n", MAX_LINE_LENGTH);
    printf("Ввод: ");
    fflush(stdout);
    
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            break;
        }
        
        // Обработка специальных символов
        if (c == 4) { // Ctrl+D
            if (pos == 0 && column == 0) {
                printf("\nПрограмма завершена.\n");
                break;
            } else {
                // Если не в начале строки, игнорируем
                sound_bell();
            }
        }
        else if (c == 127 || c == 8) { // Backspace или Delete (ERASE)
            if (pos > 0) {
                pos--;
                column--;
                printf("\b \b"); // Удаляем символ с экрана
                fflush(stdout);
            } else {
                sound_bell();
            }
        }
        else if (c == 21) { // Ctrl+U (KILL)
            if (pos > 0) {
                // Удаляем все символы в строке
                for (int i = 0; i < pos; i++) {
                    printf("\b \b");
                }
                fflush(stdout);
                pos = 0;
                column = 0;
            } else {
                sound_bell();
            }
        }
        else if (c == 23) { // Ctrl+W (DELETE WORD)
            if (pos > 0) {
                delete_last_word(line, &pos, &column);
                fflush(stdout);
            } else {
                sound_bell();
            }
        }
        else if (c == '\n' || c == '\r') { // Enter
            line[pos] = '\0';
            printf("\nВведенная строка: '%s'\n", line);
            printf("Длина: %d символов\n\n", pos);
            
            // Сбрасываем для новой строки
            pos = 0;
            column = 0;
            printf("Введите новую строку: ");
            fflush(stdout);
        }
        else if (is_printable_char(c)) { // Печатаемый символ
            if (pos < sizeof(line) - 1) {
                line[pos++] = c;
                column++;
                printf("%c", c); // Немедленно выводим символ
                fflush(stdout);
                
                // Проверяем необходимость переноса строки
                check_line_wrap(line, &pos, &column);
            } else {
                sound_bell();
            }
        }
        else { // Непечатаемый символ - звуковой сигнал
            sound_bell();
        }
    }
    
    return 0;
}