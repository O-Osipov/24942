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
        
        waitpid(pid, NULL, 0);
        printf("/n");
        printf("Последняя строка родителя\n");
    }
    
    return 0;
}