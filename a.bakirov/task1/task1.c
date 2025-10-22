#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

extern char **environ;

int main(int argc, char *argv[]) {
    char valid_opts[] = ":ispuU:cC:dvV:";  /* допустимые опции */
    int opt;

    while ((opt = getopt(argc, argv, valid_opts)) != EOF) {
        switch (opt) {
            // Печатает реальные и эффективные идентификаторы пользователя и группы
            case 'i': {
                printf("uid: %d, euid: %d, gid: %d, egid: %d\n",
                       getuid(), geteuid(), getgid(), getegid());
                break;
            }

            // Делает процесс лидером группы
            case 's': {
                if (setpgid(0, 0) == -1) {
                    perror("failed to set the group leader process\n");
                } else {
                    printf("the group leader process has been set successfully\n");
                }
                break;
            }

            // Печатает идентификаторы процесса, родителя и группы
            case 'p': {
                printf("pid: %d, ppid: %d, pgrp: %d\n",
                       getpid(), getppid(), getpgrp());
                break;
            }

            // Печатает значение ulimit
            case 'u': {
                struct rlimit limit_info;
                if (getrlimit(RLIMIT_FSIZE, &limit_info) == -1) {
                    perror("failed to get ulimit\n");
                } else {
                    printf("ulimit value: %llu\n", limit_info.rlim_max);
                }
                break;
            }

            // Изменяет значение ulimit
            case 'U': {
                long new_limit = strtol(optarg, NULL, 10);

                if (new_limit == 0) {
                    perror("invalid argument for the -U option\n");
                    break;
                }

                struct rlimit limit_info;
                if (getrlimit(RLIMIT_FSIZE, &limit_info) == -1) {
                    perror("failed to get the ulimit value\n");
                    break;
                }

                limit_info.rlim_cur = new_limit;
                if (setrlimit(RLIMIT_FSIZE, &limit_info) == -1) {
                    perror("failed to set the ulimit value\n");
                } else {
                    printf("the ulimit value has been set successfully\n");
                }
                break;
            }

            // Печатает размер core-файла
            case 'c': {
                struct rlimit core_limit;
                if (getrlimit(RLIMIT_CORE, &core_limit) == -1) {
                    perror("failed to get the core-file cap limit\n");
                } else {
                    printf("core-file cap limit: %llu\n", core_limit.rlim_max);
                }
                break;
            }

            // Изменяет размер core-файла
            case 'C': {
                long long new_core_limit = strtoll(optarg, NULL, 10);

                if (new_core_limit == 0) {
                    perror("invalid argument for the -C option\n");
                    break;
                }

                struct rlimit core_limit;
                if (getrlimit(RLIMIT_CORE, &core_limit) == -1) {
                    perror("failed to get the core-file cap limit\n");
                    break;
                }

                core_limit.rlim_cur = new_core_limit;
                if (setrlimit(RLIMIT_CORE, &core_limit) == -1) {
                    perror("failed to set the core-file cap limit\n");
                } else {
                    printf("the core-file cap limit has been set successfully\n");
                }
                break;
            }

            // Печатает текущую рабочую директорию
            case 'd': {
                char *current_dir = getenv("PWD");

                if (current_dir == NULL) {
                    perror("failed to get the current directory\n");
                } else {
                    printf("current directory: %s\n", current_dir);
                }
                break;
            }

            // Распечатывает переменные среды
            case 'v': {
                char **env_ptr = environ;
                for (; *env_ptr != NULL; env_ptr++) {
                    printf("%s\n", *env_ptr);
                }
                break;
            }

            // Устанавливает новую переменную окружения
            case 'V': {
                if (putenv(optarg) == -1) {
                    printf("failed to set the environmental variable");
                }
                break;
            }

            // Отсутствует аргумент
            case ':': {
                printf("missing argument for option: %c\n", optopt);
                break;
            }

            // Неизвестная опция
            case '?':
            default:
                printf("invalid option: %c\n", optopt);
        }
    }

    return 0;
}

