#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Глобальная переменная для подсчета количества звуковых сигналов
int beep_count = 0;

// Функция-обработчик сигнала SIGINT (Ctrl+C)
void siginth(int sig) {
    printf("\nBeep count: %d\n", beep_count);
    exit(0);
}

int main() {
    // Регистрируем обработчик сигнала для Ctrl+C
    signal(SIGINT, siginth);
    
    printf("Program started. Type text and press Enter to beep. Press CTRL-D to exit.\n");
    
    while(1) {
        // Чтение одного символа из стандартного ввода
        // getchar() ждет, пока пользователь введет данные и нажмет Enter
        // Затем читает по одному символу из буфера ввода
        int c = getchar();
        
        // Проверка на конец файла (Ctrl+D в Linux/Unix)
        // Когда пользователь нажимает Ctrl+D, getchar() возвращает EOF
        if (c == EOF) {
            printf("\nBeep count: %d\n", beep_count);  // Выводим статистику
            exit(0);  // Завершаем программу
        }
        
        //  Проверка на символ новой строки (Enter)
        // Когда пользователь нажимает Enter, в буфере появляется символ '\n'
        if (c == '\n') {
            // Издаем звуковой сигнал
            // \a - это специальный символ bell (звонок)
            write(1, "\a", 1); 
            
            // Увеличиваем счетчик писков
            beep_count++;
            
            // Выводим сообщение о выполнении строки
            printf("Beep! (line completed)\n");
        }
        
        // Если символ не Enter и не EOF, программа просто переходит к чтению следующего символа
        // Например: если ввели "hello", то будут прочитаны 'h','e','l','l','o','\n'
        // Писк произойдет только при чтении '\n'
    }
    
    return 0;
}