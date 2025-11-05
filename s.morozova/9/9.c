#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        execlp("cat", "cat", "long_file.txt", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        printf("Родитель печатает текст\n");
        
        waitpid(pid, NULL, 0);
        printf("Последняя строка родителя\n");
    }
    
    return 0;
}