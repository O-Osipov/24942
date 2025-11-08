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
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(pipefd[1]);
        
        printf("Дочерний процесс готов к преобразованию текста...\n");
        
        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, "Преобразованный текст: ", 23);
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
        
    } else {
        close(pipefd[0]);
        
        printf("Родительский процесс. Введите текст (Ctrl+D для завершения):\n");
        
        while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
            write(pipefd[1], buffer, bytes_read);
        }
        
        close(pipefd[1]);
        wait(NULL);
    }
    
    return 0;
}