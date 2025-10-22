#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE 256

typedef struct Node_s {
    char *string;
    struct Node_s *next;
} Node;

Node *head, *tail;

void init() {
    head = malloc(sizeof(Node));
    head->string = NULL;
    head->next = NULL;
    tail = head;
}

void push(char *string) {
    if (string == NULL || *string == '\0') return;
    
    unsigned long len = strlen(string) + 1;
    char *copyPtr = malloc(len);
    strcpy(copyPtr, string);
    
    tail->string = copyPtr;
    tail->next = calloc(1, sizeof(Node));
    tail = tail->next;
}

void printList() {
    Node *ptr = head;
    int count = 0;
    printf("Содержимое списка:\n");
    while (ptr != NULL && ptr->string != NULL) {
        printf("%s\n", ptr->string);
        ptr = ptr->next;
        count++;
    }
    if (count == 0) {
        printf("(список пуст)\n");
    }
}

// Функция для чтения одной строки БЕЗ обработки escape-последовательностей
char* read_raw_input() {
    static char buffer[BUFFER_SIZE];
    int i = 0;
    char c;
    
    while (1) {
        if (read(STDIN_FILENO, &c, 1) != 1) {
            return NULL;
        }
        
        // Enter - завершаем ввод
        if (c == '\n' || c == '\r') {
            buffer[i] = '\0';
            return buffer;
        }
        
        // Backspace
        if (c == 127 || c == 8) {
            if (i > 0) {
                i--;
                printf("\b \b"); // Стираем символ в терминале
                fflush(stdout);
            }
            continue;
        }
        
        // Обычный символ
        if (i < BUFFER_SIZE - 1 && c >= 32 && c <= 126) {
            buffer[i++] = c;
            putchar(c); // Эхо символа
            fflush(stdout);
        }
    }
}

// Настройки терминала
struct termios original_termios;

void disable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &original_termios);
    term = original_termios;
    term.c_lflag &= ~(ICANON | ECHO); // Отключаем канонический режим и эхо
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

int main() {
    init();
    
    // Отключаем обработку специальных клавиш
    disable_echo();
    
    printf("Вводите строки (одиночная точка для выхода):\n");
    
    // Очищаем stdin перед началом
    tcflush(STDIN_FILENO, TCIFLUSH);
    
    while (1) {
        printf("\n> ");
        fflush(stdout);
        
        char *input = read_raw_input();
        if (input == NULL) {
            printf("\nЗавершение работы.\n");
            break;
        }
        
        // Проверяем на точку
        if (strcmp(input, ".") == 0) {
            printf("\n");
            printList();
            break;
        }
        
        // Добавляем в список если строка не пустая
        if (strlen(input) > 0) {
            push(input);
            printf(" [добавлено]");
        }
    }
    
    restore_terminal();
    return 0;
}
