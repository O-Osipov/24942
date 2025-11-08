#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Создаем pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Создаем подпроцесс
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // ДОЧЕРНИЙ ПРОЦЕСС: получает и преобразует текст
        close(pipefd[1]); // Закрываем конец для записи
        
        printf("Дочерний процесс: получаю текст и преобразую в верхний регистр...\n");
        
        // Читаем и преобразуем данные
        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
        
    } else {
        // РОДИТЕЛЬСКИЙ ПРОЦЕСС: отправляет смешанный текст
        close(pipefd[0]); // Закрываем конец для чтения
        
        const char *mixed_text = "Hello World!\n"
                                "This Is A Mixed Case Text.\n"
                                "programming in C is FUN!\n"
                                "Linux POSIX API\n"
                                "End Of Message.\n";
        
        printf("Родительский процесс: отправляю смешанный текст в канал\n");
        write(pipefd[1], mixed_text, strlen(mixed_text));
        
        close(pipefd[1]);
        wait(NULL);
        printf("Родительский процесс: работа завершена\n");
    }
    
    return 0;
}