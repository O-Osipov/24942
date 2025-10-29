#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

// Функция для обработки встроенных команд
int handle_builtin(struct command *cmd) {
    if (strcmp(cmd->cmdargs[0], "cd") == 0) {
        char *path = cmd->cmdargs[1];
        if (path == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        if (chdir(path) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }
    else if (strcmp(cmd->cmdargs[0], "exit") == 0) {
        exit(0);
    }
    return -1; // Не встроенная команда
}

main(int argc, char *argv[])
{
    register int i;
    char line[1024];
    int ncmds;
    char prompt[50];
    pid_t pid;
    int status;
    int fd_in, fd_out;
    int pipefd[2];
    int in_fd = 0;

    sprintf(prompt,"[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {
        if ((ncmds = parseline(line)) <= 0)
            continue;

        in_fd = 0;

        // Сохраняем значения перенаправлений перед использованием
        char *saved_infile = infile;
        char *saved_outfile = outfile;
        char *saved_appfile = appfile;
        char saved_bkgrnd = bkgrnd;

        // Handle input redirection for the entire command chain
        if (infile != NULL) {
            in_fd = open(infile, O_RDONLY);
            if (in_fd < 0) {
                perror("open input file");
                infile = NULL;
                continue;
            }
        }

        // Если только одна команда и она встроенная - выполняем без fork
        if (ncmds == 1 && handle_builtin(&cmds[0]) != -1) {
            if (in_fd > 0) close(in_fd);
            // Сбрасываем перенаправления после выполнения команды
            infile = outfile = appfile = NULL;
            bkgrnd = 0;
            continue;
        }

        for (i = 0; i < ncmds; i++) {
            // Пропускаем встроенные команды в пайпах
            if (ncmds > 1 && handle_builtin(&cmds[i]) != -1) {
                fprintf(stderr, "Builtin commands don't work in pipes: %s\n", cmds[i].cmdargs[0]);
                break;
            }

            // Create pipe for all commands except the last one
            if (i < ncmds - 1) {
                if (pipe(pipefd) < 0) {
                    perror("pipe failed");
                    if (in_fd > 0) close(in_fd);
                    continue;
                }
            }

            pid = fork();
            
            if (pid == 0) {  /* child process */
                // Set up input
                if (in_fd != 0) {
                    dup2(in_fd, 0);
                    close(in_fd);
                }

                // Set up output
                if (i < ncmds - 1) {
                    close(pipefd[0]);
                    dup2(pipefd[1], 1);
                    close(pipefd[1]);
                } else {
                    // Для последней команды используем сохраненные значения
                    if (saved_outfile != NULL) {
                        fd_out = open(saved_outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd_out < 0) {
                            perror("open output file");
                            exit(1);
                        }
                        dup2(fd_out, 1);
                        close(fd_out);
                    } else if (saved_appfile != NULL) {
                        fd_out = open(saved_appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (fd_out < 0) {
                            perror("open append file");
                            exit(1);
                        }
                        dup2(fd_out, 1);
                        close(fd_out);
                    }
                }

                // Close all other file descriptors
                if (in_fd > 0) close(in_fd);
                if (i < ncmds - 1) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }

                // Execute command
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror("execvp failed");
                exit(1);
            }
            else if (pid < 0) {
                perror("fork failed");
                if (in_fd > 0) close(in_fd);
                if (i < ncmds - 1) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }
                continue;
            }

            // Parent process cleanup
            if (in_fd != 0) {
                close(in_fd);
                in_fd = 0;
            }

            if (i < ncmds - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        }

        if (in_fd > 0) {
            close(in_fd);
        }

        // Wait for all children in foreground
        if (!saved_bkgrnd) {
            for (i = 0; i < ncmds; i++) {
                wait(&status);
            }
        }

        // Reset for next command
        infile = outfile = appfile = NULL;
        bkgrnd = 0;

    }
}