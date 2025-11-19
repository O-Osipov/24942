#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        execlp("cat", "cat", "test.txt", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        printf("Родитель печатает текст\n");
        
        waitid(P_PID, pid, NULL, WEXITED);
        printf("Последняя строка родителя\n");
    }
    
    return 0;
}