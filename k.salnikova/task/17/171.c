#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

// Константы для ограничений редактора
#define MAX_LINE_LENGTH 40        // Максимальная длина строки
#define MAX_TEXT_LENGTH 2000      // Максимальная длина всего текста
#define BELL '\007'               // Символ звонка (BEL)
#define ERASE 0x7F                // Код клавиши Backspace/Delete
#define KILL 0x15                 // Ctrl+U - удалить всю строку до курсора
#define CTRL_W 0x17               // Ctrl+W - удалить слово
#define CTRL_D 0x04               // Ctrl+D - выход

// Глобальная переменная для хранения оригинальных настроек терминала
struct termios original_termios;

// Функция восстановления оригинальных настроек терминала
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

// Функция настройки терминала в неканонический режим
void setup_terminal(void) {
    struct termios new_termios;
    // Получаем текущие настройки терминала
    tcgetattr(STDIN_FILENO, &original_termios);
    // Регистрируем функцию восстановления для вызова при выходе
    atexit(restore_terminal);
    
    // Копируем настройки и модифицируем их
    new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  // Отключаем канонический режим и эхо
    new_termios.c_cc[VMIN] = 1;               // Минимальное количество символов для чтения
    new_termios.c_cc[VTIME] = 0;              // Таймаут чтения (0 - бесконечно)
    
    // Применяем новые настройки
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

// Структура для хранения состояния текстового редактора
struct editor {
    char text[MAX_TEXT_LENGTH];  // Буфер для хранения текста
    int pos;                     // Текущая позиция курсора в тексте
    int len;                     // Длина текста в буфере
};

// Функция перерисовки экрана редактора
void redraw(struct editor *e) {
    // Сохраняем позицию курсора, перемещаемся в начало и очищаем строку
    printf("\033[s\033[1;1H\033[2KInput text (CTRL-D at line start to exit):");
    // Переходим на вторую строку и очищаем экран от курсора до конца
    printf("\033[2;1H\033[J");
    
    // Если есть текст, выводим его с переносами слов
    if (e->len > 0) {
        int col = 0;  // Текущая колонка в строке
        for (int i = 0; i < e->len; i++) {
            putchar(e->text[i]);
            col++;
            
            // Проверяем, нужно ли сделать перенос строки
            if (col >= MAX_LINE_LENGTH && e->text[i] != ' ') {
                // Ищем начало текущего слова для переноса
                int start = i;
                while (start > 0 && e->text[start - 1] != ' ') start--;
                
                // Переносим, если слово слишком длинное или следующее не пробел
                if (i - start >= MAX_LINE_LENGTH || 
                    (i + 1 < e->len && e->text[i + 1] != ' ')) {
                    printf("\n");
                    col = 0;
                }
            } else if (col >= MAX_LINE_LENGTH) {
                // Перенос по достижению максимальной длины строки
                printf("\n");
                col = 0;
            }
        }
    }
    
    // Вычисляем позицию курсора для установки
    int line = 2, col = 0;  // Начинаем со второй строки
    for (int i = 0; i < e->pos; i++) {
        col++;
        // Повторяем логику переноса для вычисления позиции курсора
        if (col >= MAX_LINE_LENGTH && e->text[i] != ' ') {
            int start = i;
            while (start > 0 && e->text[start - 1] != ' ') start--;
            if (i - start >= MAX_LINE_LENGTH || 
                (i + 1 < e->len && e->text[i + 1] != ' ')) {
                line++;
                col = 0;
            }
        } else if (col >= MAX_LINE_LENGTH) {
            line++;
            col = 0;
        }
    }
    
    // Устанавливаем курсор в вычисленную позицию
    printf("\033[%d;%dH", line, col + 1);
    fflush(stdout);  // Принудительно выводим буфер
}

// Функция удаления слова перед курсором
void erase_word(struct editor *e) {
    if (e->pos == 0) {
        putchar(BELL);  // Звонок, если нечего удалять
        fflush(stdout);
        return;
    }
    
    // Находим начало слова для удаления
    int end = e->pos;
    // Пропускаем пробелы и табы
    while (end > 0 && (e->text[end - 1] == ' ' || e->text[end - 1] == '\t')) 
        end--;
    // Находим начало слова
    while (end > 0 && e->text[end - 1] != ' ' && e->text[end - 1] != '\t') 
        end--;
    
    // Удаляем слово
    int n = e->pos - end;  // Количество символов для удаления
    if (n > 0) {
        // Сдвигаем оставшийся текст на место удаленного
        memmove(e->text + end, e->text + e->pos, e->len - e->pos + 1);
        e->len -= n;
        e->pos = end;
        redraw(e);  // Перерисовываем экран
    }
}

int main(void) {
    // Инициализируем редактор
    struct editor e = { .pos = 0, .len = 0 };
    char c;  // Буфер для вводимого символа
    
    setup_terminal();  // Настраиваем терминал
    // Очищаем экран и выводим приглашение
    printf("\033[2J\033[HInput text (CTRL-D at line start to exit):\n");
    fflush(stdout);
    
    // Главный цикл обработки ввода
    while (read(STDIN_FILENO, &c, 1) == 1) {
        // Обработка выхода (Ctrl+D в начале строки)
        if (c == CTRL_D && e.pos == 0) {
            printf("\nExit.\n");
            break;
        }
        
        // Обработка Backspace/Delete
        if (c == ERASE) {
            if (e.pos > 0) {
                e.pos--;
                e.len--;
                // Сдвигаем текст на место удаленного символа
                memmove(e.text + e.pos, e.text + e.pos + 1, e.len - e.pos + 1);
                redraw(&e);
            } else {
                putchar(BELL);  // Звонок, если нечего удалять
                fflush(stdout);
            }
            continue;
        }
        
        // Обработка Ctrl+U (удалить до начала строки)
        if (c == KILL) {
            if (e.pos > 0) {
                // Сдвигаем текст на место удаленной части
                memmove(e.text, e.text + e.pos, e.len - e.pos + 1);
                e.len -= e.pos;
                e.pos = 0;
                redraw(&e);
            } else {
                putchar(BELL);
                fflush(stdout);
            }
            continue;
        }
        
        // Обработка Ctrl+W (удалить слово)
        if (c == CTRL_W) {
            erase_word(&e);
            continue;
        }
        
        // Игнорируем непечатаемые символы (кроме уже обработанных)
        if (c < 32 || c > 126) {
            putchar(BELL);
            fflush(stdout);
            continue;
        }
        
        // Проверка переполнения буфера
        if (e.len >= MAX_TEXT_LENGTH - 1) {
            putchar(BELL);
            fflush(stdout);
            continue;
        }
        
        // Вставка обычного символа
        if (e.pos < e.len) {
            // Сдвигаем текст для вставки нового символа
            memmove(e.text + e.pos + 1, e.text + e.pos, e.len - e.pos);
        }
        e.text[e.pos] = c;  // Вставляем символ
        e.pos++;
        e.len++;
        e.text[e.len] = '\0';  // Завершаем строку
        redraw(&e);  // Перерисовываем экран
    }
    
    return 0;
}