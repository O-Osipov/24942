#include <stdio.h> 
#include <unistd.h> //для posix-функции(fork, exec)
#include <sys/wait.h> //функция ожидания дочерних процессов 
#include <string.h> 
#include <stdlib.h> //exit, malloc, free ...

int main(int argc, char *argv[]){
    //проверка что передан ровно один аргумент - имя файла
    if (argc != 2){
        //печатаем корректный формат запуска
        printf("Usage: %s <filename>\n", argv[0]);
        printf("Example: %s test.txt\n", argv[0]);
        return 1;
    }
    //сохраняем имя файла из аргумента 
    char *filename = argv[1];
    //создаем подпроцесс
    pid_t pid = fork();
    //переменная для хранения статуса завершения дочерного процесса 
    int status; 

    //проверка на ошибку форк
    if (pid == -1){
        perror("fork failed");
        return 1;
    }

    //это выполняется в дочернем процессе 
    else if (pid == 0){
        printf("Child process (PID: %d) executing cat on file: %s\n", getpid(), filename);

        //исполняем cat fileneme
        //execlp  ЗАМЕНЯЕТ ТЕКУЩИЙ ПРОЦЕСС ->если успешен, ниже код не выполняется
        execpl("cat", "cat", filename, NULL);

        //если дошли сюда - exec не сработал 
        perror("execpl failed");
        exit(1); 
    }
    //это выполняется в родителе
    else {
        printf("parent process (PID: %d) created child(PID: %d)\n", getpid(), pid);

        //ждем завершения дочернего процесса 
        if (waitpid(pid, &status, 0) == - 1){
            perror("waitpid failed");
            return 1; 
        }
        //проверяем нормально ли завершился дочерний процесс
        if (WIFEXITED(status)){
            printf("\nChild process completed successfully with exit code: %d\n", WEXITSTATUS(status));
        } else {
            printf("\nChild process terminated abnormally\n");
        }
        //это строка гарантированно печатается ПОСЛЕ завершения дочернего процесса
        printf("\n PARENT's FINAL MESSAGE: Child process has finished!\n");
    }

    return 0; 
}

