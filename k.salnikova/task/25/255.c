#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main() {
    int pipefd[2];  // Массив для файловых дескрипторов канала [0] - чтение, [1] - запись
    pid_t pid;      // Переменная для хранения ID процесса
    char buffer[BUFFER_SIZE];  // Буфер для чтения/записи данных
    ssize_t bytes_read;        // Количество прочитанных байт
    
    // Создаем программный канал (pipe)
    if (pipe(pipefd) == -1) {
        perror("pipe");  // Выводим ошибку если не удалось создать канал
        exit(EXIT_FAILURE);
    }
    
    // Создаем дочерний процесс (подпроцесс)
    pid = fork();
    if (pid == -1) {
        perror("fork");  // Ошибка при создании процесса
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Этот код выполняется в ДОЧЕРНЕМ ПРОЦЕССЕ
        
        close(pipefd[1]);  // Закрываем конец для ЗАПИСИ (дочерний только читает)
        
        // Читаем данные из канала пока они есть
        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            // Преобразуем каждый символ в верхний регистр
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            // Выводим преобразованный текст на терминал
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]);  // Закрываем конец для ЧТЕНИЯ
        exit(EXIT_SUCCESS);  // Завершаем дочерний процесс
        
    } else {
        // Этот код выполняется в РОДИТЕЛЬСКОМ ПРОЦЕССЕ
        
        close(pipefd[0]);  // Закрываем конец для ЧТЕНИЯ (родитель только пишет)
        
        // Текст который будем отправлять (смесь верхнего и нижнего регистра)
        const char *text = "Hello World!\n"
                          "This is a Test String with Mixed CASE letters.\n"
                          "Programming in C is FUN!\n"
                          "End of transmission.\n";
        
        // Записываем текст в канал
        write(pipefd[1], text, strlen(text));
        
        close(pipefd[1]);  // Закрываем конец для ЗАПИСИ (сигнал конца данных)
        
        wait(NULL);  // Ждем завершения дочернего процесса
    }
    
    return 0;
}