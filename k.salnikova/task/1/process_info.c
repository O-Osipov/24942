#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <linux/limits.h>

/*
 * Функция для печати идентификаторов пользователя и группы
 */
void print_user_group_ids() {
    printf("=== User and Group IDs ===\n");
    printf("Real User ID (RUID): %d\n", getuid());
    printf("Effective User ID (EUID): %d\n", geteuid());
    printf("Real Group ID (RGID): %d\n", getgid());
    printf("Effective Group ID (EGID): %d\n", getegid());
}

/*
 * Функция делает процесс лидером новой группы процессов
 */
void become_process_group_leader() {
    printf("=== Process Group Leadership ===\n");
    if (setpgid(0, 0) == 0) {
        printf("Стал лидером новой группы процессов. Новый PGID: %d\n", getpgrp());
    } else {
        perror("Ошибка при установке лидера группы");
    }
}

/*
 * Печатает основные идентификаторы процесса
 */
void print_process_ids() {
    printf("=== Process Identification ===\n");
    printf("Process ID (PID): %d\n", getpid());
    printf("Parent Process ID (PPID): %d\n", getppid());
    printf("Process Group ID (PGID): %d\n", getpgrp());
}

/*
 * Получает и печатает текущее значение ulimit для размера файлов
 */
void print_ulimit() {
    printf("=== File Size Limit (ulimit) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_FSIZE, &rlim) == 0) {
        printf("Текущий ulimit (макс. размер файла): ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении ulimit");
    }
}

/*
 * Устанавливает новое значение ulimit
 */
int set_new_ulimit(const char *value) {
    printf("=== Setting New Ulimit ===\n");
    struct rlimit rlim;
    long new_limit;
    char *endptr;
    
    new_limit = strtol(value, &endptr, 10);
    
    if (endptr == value) {
        fprintf(stderr, "Ошибка: '%s' не является числом\n", value);
        return -1;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: нечисловые символы в '%s'\n", value);
        return -1;
    }
    if (new_limit < 0) {
        fprintf(stderr, "Ошибка: ulimit не может быть отрицательным\n");
        return -1;
    }
    
    if (getrlimit(RLIMIT_FSIZE, &rlim) != 0) {
        perror("Ошибка при получении текущего ulimit");
        return -1;
    }
    
    rlim.rlim_cur = (new_limit < rlim.rlim_max) ? new_limit : rlim.rlim_max;
    
    if (setrlimit(RLIMIT_FSIZE, &rlim) != 0) {
        perror("Ошибка при установке ulimit");
        return -1;
    }
    
    printf("Ulimit успешно установлен: %ld байт\n", new_limit);
    return 0;
}

void print_core_size() {
    printf("=== Core File Size Limit ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        printf("Максимальный размер core-файла: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении размера core-файла");
    }
}

/*
 * Устанавливает новый размер для core-файлов
 */
int set_core_size(const char *value) {
    printf("=== Setting Core File Size ===\n");
    struct rlimit rlim;
    long new_size;
    char *endptr;
    
    new_size = strtol(value, &endptr, 10);
    
    if (endptr == value || *endptr != '\0' || new_size < 0) {
        fprintf(stderr, "Ошибка: неверный размер core-файла '%s'\n", value);
        return -1;
    }
    
    if (getrlimit(RLIMIT_CORE, &rlim) != 0) {
        perror("Ошибка при получении текущего размера core-файла");
        return -1;
    }
    
    rlim.rlim_cur = (new_size < rlim.rlim_max) ? new_size : rlim.rlim_max;
    
    if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
        perror("Ошибка при установке размера core-файла");
        return -1;
    }
    
    printf("Размер core-файла установлен: %ld байт\n", new_size);
    return 0;
}

void print_current_directory() {
    printf("=== Current Working Directory ===\n");
    char cwd[PATH_MAX];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Текущая директория: %s\n", cwd);
    } else {
        perror("Ошибка при получении текущей директории");
    }
}


void print_environment() {
    printf("=== Environment Variables ===\n");
    extern char **environ;
    
    if (environ == NULL || environ[0] == NULL) {
        printf("Нет переменных окружения\n");
        return;
    }
    
    char **env = environ;
    int count = 0;
    
    while (*env != NULL) {
        printf("%d: %s\n", ++count, *env);
        env++;
    }
}

/*
 * Устанавливает новую переменную окружения или изменяет существующую
 */
int set_environment_variable(const char *name_value) {
    printf("=== Setting Environment Variable ===\n");
    char *name = strdup(name_value);
    if (name == NULL) {
        perror("Ошибка выделения памяти");
        return -1;
    }
    
    char *value = strchr(name, '=');
    
    if (value == NULL) {
        fprintf(stderr, "Ошибка: формат должен быть 'name=value', получено: '%s'\n", name_value);
        free(name);
        return -1;
    }
    
    *value = '\0';
    value++;
    
    if (setenv(name, value, 1) != 0) {
        perror("Ошибка при установке переменной окружения");
        free(name);
        return -1;
    }
    
    printf("Установлена переменная окружения: %s=%s\n", name, value);
    free(name);
    return 0;
}


typedef struct {
    int opt;
    char *arg;
} option_t;

int main(int argc, char *argv[]) {
    printf("=== Process Information Tool ===\n");
    
    if (argc == 1) {
        printf("Использование: %s [опции...]\n", argv[0]);
        printf("Опции (обрабатываются справа налево):\n");
        printf("  -i      Показать ID пользователя и группы\n");
        printf("  -s      Стать лидером группы процессов\n");
        printf("  -p      Показать ID процесса\n");
        printf("  -u      Показать ulimit\n");
        printf("  -U size Установить ulimit\n");
        printf("  -c      Показать размер core-файла\n");
        printf("  -C size Установить размер core-файла\n");
        printf("  -d      Показать текущую директорию\n");
        printf("  -v      Показать переменные окружения\n");
        printf("  -V var=value Установить переменную окружения\n");
        return 0;
    }

    // Массив для хранения опций в порядке выполнения (справа налево)
    option_t *options = malloc(argc * sizeof(option_t));
    int option_count = 0;
    
    printf("Обработка опций справа налево...\n\n");

    // ПЕРВЫЙ ПРОХОД: собираем все опции с помощью getopt в обратном порядке
    for (int start_pos = argc - 1; start_pos >= 1; start_pos--) {
        // Если текущая позиция - это опция
        if (argv[start_pos][0] == '-') {
            // Временные переменные для работы getopt
            int saved_optind = optind;
            int saved_opterr = opterr;
            
            // Устанавливаем начальную позицию для getopt
            optind = start_pos;
            opterr = 0; // Отключаем стандартные сообщения об ошибках
            
            int opt;
            while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
                if (opt != '?') { // Игнорируем неизвестные опции на этом этапе
                    options[option_count].opt = opt;
                    options[option_count].arg = (optarg != NULL) ? strdup(optarg) : NULL;
                    option_count++;
                    break; // Обрабатываем только одну опцию за раз
                }
            }
            
            optind = saved_optind;
            opterr = saved_opterr;
        }
    }

    // ВТОРОЙ ПРОХОД: выполняем опции в собранном порядке (справа налево)
    for (int i = 0; i < option_count; i++) {
        switch (options[i].opt) {
            case 'i':
                print_user_group_ids();
                break;
            case 's':
                become_process_group_leader();
                break;
            case 'p':
                print_process_ids();
                break;
            case 'u':
                print_ulimit();
                break;
            case 'U':
                if (options[i].arg != NULL) {
                    set_new_ulimit(options[i].arg);
                } else {
                    fprintf(stderr, "Ошибка: опция -U требует значение\n");
                }
                break;
            case 'c':
                print_core_size();
                break;
            case 'C':
                if (options[i].arg != NULL) {
                    set_core_size(options[i].arg);
                } else {
                    fprintf(stderr, "Ошибка: опция -C требует значение\n");
                }
                break;
            case 'd':
                print_current_directory();
                break;
            case 'v':
                print_environment();
                break;
            case 'V':
                if (options[i].arg != NULL) {
                    set_environment_variable(options[i].arg);
                } else {
                    fprintf(stderr, "Ошибка: опция -V требует значение\n");
                }
                break;
            case '?':
                break;
        }
        printf("\n");

        if (options[i].arg != NULL) {
            free(options[i].arg);
        }
    }

    free(options);
    return 0;
}


/*
chmod +x test_process_info.sh
./test_process_info.sh 
*/