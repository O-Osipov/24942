#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int fildes[2];

    if (pipe(fildes) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // === Дочерний процесс ===
        // Читает из канала, переводит в верхний регистр и выводит на экран
        close(fildes[1]);  // не пишет в канал

        char buf[1024];
        ssize_t nread;

        while ((nread = read(fildes[0], buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < nread; ++i) {
                buf[i] = (char) toupper((unsigned char) buf[i]);
            }

            if (write(STDOUT_FILENO, buf, nread) == -1) {
                perror("write to stdout");
                close(fildes[0]);
                _exit(1);
            }
        }

        if (nread == -1) {
            perror("read from pipe");
        }

        close(fildes[0]);
        _exit(0);
    } else {
        // === Родительский процесс ===
        // Читает текст с stdin и отправляет его в канал
        close(fildes[0]);  // не читает из канала

        printf("=== Программа связи через программный канал ===\n");
        printf("Введите любой текст (смешанный регистр).\n");
        printf("Программа переведёт все символы в верхний регистр.\n");
        printf("Для завершения ввода нажмите Ctrl+D (Linux/Mac) или Ctrl+Z (Windows WSL):\n\n");

        char buf[1024];
        ssize_t nread;

        // Читаем всё, что подадут на stdin, и пишем в pipe
        while ((nread = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            if (write(fildes[1], buf, nread) == -1) {
                perror("write to pipe");
                close(fildes[1]);
                waitpid(pid, NULL, 0);
                return 1;
            }
        }

        if (nread == -1) {
            perror("read from stdin");
        }

        close(fildes[1]);      // закрываем запись, чтобы дочерний увидел EOF
        waitpid(pid, NULL, 0); // ждём завершения дочернего процесса
        printf("\n\n=== Завершено. ===\n");
    }

    return 0;
}

