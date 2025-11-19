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
    FILE *file;
    
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
        close(pipefd[0]);
        
        file = fopen("text.txt", "r");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        
        printf("Дочерний процесс: читаю текст из text.txt и отправляю в канал\n");
        
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            write(pipefd[1], buffer, bytes_read);
        }
        
        fclose(file);
        close(pipefd[1]);
        exit(EXIT_SUCCESS);
        
    } else {
        close(pipefd[1]);
        
        printf("Родительский процесс: получаю текст и преобразую в верхний регистр...\n\n");
        
        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]);
        wait(NULL);
        printf("\nОба процесса завершили работу.\n");
    }
    
    return 0;
}