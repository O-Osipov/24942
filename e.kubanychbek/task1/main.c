#include <stdio.h>      // Стандартный ввод-вывод (printf, perror)
#include <stdlib.h>     // Стандартные функции (atoi, strdup, free)
#include <unistd.h>     // POSIX API (getopt, getpid, getcwd, etc.)
#include <sys/types.h>  // Типы данных системных вызовов (pid_t, uid_t)
#include <sys/stat.h>   // Статус файлов
#include <sys/resource.h> // Лимиты ресурсов (getrlimit, setrlimit)
#include <pwd.h>        // Работа с паролями пользователей
#include <grp.h>        // Работа с группами
#include <string.h>     // Работа со строками (strdup)


extern char *optarg;
extern int optopt;
// Функция для печати реальных и эффективных идентификаторов пользователя и группы
void print_user_ids() {
    printf("Real UID: %d\n", getuid());        // Реальный ID пользователя
    printf("Effective UID: %d\n", geteuid());  // Эффективный ID пользователя
    printf("Real GID: %d\n", getgid());        // Реальный ID группы
    printf("Effective GID: %d\n", getegid());  // Эффективный ID группы
}

// Функция для создания процесса лидером группы
void make_session_leader() {
    // setpgid(0, 0) - установить текущий процесс (pid=0) лидером новой группы
    if (setpgid(0, 0) == -1) {
        perror("setpgid failed");  // Вывод ошибки если не удалось
    } else {
        printf("Process became session leader\n");  // Успешное выполнение
    }
}

// Функция для печати идентификаторов процесса
void print_process_ids() {
    printf("Process ID (PID): %d\n", getpid());      // ID текущего процесса
    printf("Parent Process ID (PPID): %d\n", getppid()); // ID родительского процесса
    printf("Process Group ID (PGID): %d\n", getpgrp());  // ID группы процессов
}

// Функция для печати текущего значения ulimit (ограничение размера файла)
void print_ulimit() {
    struct rlimit rlim;  // Структура для хранения лимитов ресурсов
    // RLIMIT_FSIZE - максимальный размер файла, который может создать процесс
    if (getrlimit(RLIMIT_FSIZE, &rlim) == 0) {
        printf("Ulimit (file size): %ld\n", (long)rlim.rlim_cur);  // Текущее значение
    } else {
        perror("getrlimit failed");  // Ошибка получения лимита
    }
}

// Функция для установки нового значения ulimit
void set_ulimit(const char *value) {
    long new_limit = atol(value);  // Преобразование строки в число
    struct rlimit rlim;
    rlim.rlim_cur = new_limit;  // Текущее значение лимита
    rlim.rlim_max = new_limit;  // Максимальное значение лимита
    
    // Установка нового лимита для размера файла
    if (setrlimit(RLIMIT_FSIZE, &rlim) == -1) {
        perror("setrlimit failed");  // Ошибка установки лимита
    } else {
        printf("Ulimit set to: %ld\n", new_limit);  // Успешная установка
    }
}

// Функция для печати размера core-файла
void print_core_size() {
    struct rlimit rlim;
    // RLIMIT_CORE - максимальный размер core-файла
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        printf("Core file size: %ld bytes\n", (long)rlim.rlim_cur);
    } else {
        perror("getrlimit failed");
    }
}

// Функция для установки размера core-файла
void set_core_size(const char *value) {
    long new_size = atol(value);  // Преобразование строки в число
    struct rlimit rlim;
    rlim.rlim_cur = new_size;  // Текущий размер core-файла
    rlim.rlim_max = new_size;  // Максимальный размер core-файла
    
    if (setrlimit(RLIMIT_CORE, &rlim) == -1) {
        perror("setrlimit failed");
    } else {
        printf("Core file size set to: %ld bytes\n", new_size);
    }
}

// Функция для печати текущей рабочей директории
void print_current_directory() {
    // Используем фиксированный размер буфера если PATH_MAX не определен
    #ifdef PATH_MAX
    char cwd[PATH_MAX];  // Буфер для пути
    #else
    char cwd[4096];      // Альтернативный размер буфера
    #endif
    
    // getcwd - получение текущей рабочей директории
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current directory: %s\n", cwd);
    } else {
        perror("getcwd failed");  // Ошибка получения директории
    }
}

// Функция для печати всех переменных окружения
void print_environment() {
    extern char **environ;  // Внешняя переменная - массив переменных окружения
    char **env = environ;   // Указатель на начало массива
    
    printf("Environment variables:\n");
    // Перебор всех переменных окружения (последний элемент - NULL)
    while (*env != NULL) {
        printf("  %s\n", *env);  // Печать каждой переменной
        env++;  // Переход к следующей переменной
    }
}

// Функция для установки переменной окружения
void set_environment_variable(const char *name_value) {
    // Копируем строку, так как putenv требует неконстантную строку
    char *env_var = strdup(name_value);  // Дублирование строки в кучу
    if (env_var == NULL) {
        perror("strdup failed");  // Ошибка выделения памяти
        return;
    }
    
    // putenv добавляет переменную в окружение процесса
    if (putenv(env_var) != 0) {
        perror("putenv failed");  // Ошибка установки переменной
        free(env_var);  // Освобождаем память при ошибке
    } else {
        printf("Environment variable set: %s\n", name_value);
        // Память не освобождается - она теперь принадлежит environ
    }
}

int main(int argc, char *argv[]) {
    int opt;  // Переменная для хранения текущей опции
    
    // Если нет аргументов, просто выходим
    if (argc == 1) {
        printf("No arguments provided. Use -h for help.\n");
        return 0;
    }
    
    // Упрощенная обработка опций - обрабатываем все аргументы по порядку
    // но выполняем функции в порядке обратном порядку аргументов
    int *option_stack = malloc(argc * sizeof(int));
    char **option_args = malloc(argc * sizeof(char*));
    int option_count = 0;
    
    // Сначала собираем все опции в стек
    while ((opt = getopt(argc, argv, "ispucdvhU:C:V:")) != -1) {
        switch (opt) {
            case 'i': case 's': case 'p': case 'u': case 'c': case 'd': case 'v': case 'h':
                option_stack[option_count] = opt;
                option_args[option_count] = NULL;
                option_count++;
                break;
            case 'U': case 'C': case 'V':
                option_stack[option_count] = opt;
                option_args[option_count] = optarg;
                option_count++;
                break;
            case '?':  // Неизвестная опция
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                break;
        }
    }
    
    // Теперь выполняем опции в обратном порядке (LIFO - стек)
    for (int i = option_count - 1; i >= 0; i--) {
        switch (option_stack[i]) {
            case 'i':  // Печать идентификаторов пользователя/группы
                print_user_ids();
                break;
            case 's':  // Сделать процесс лидером группы
                make_session_leader();
                break;
            case 'p':  // Печать идентификаторов процесса
                print_process_ids();
                break;
            case 'u':  // Печать ulimit
                print_ulimit();
                break;
            case 'U':  // Установка ulimit
                set_ulimit(option_args[i]);
                break;
            case 'c':  // Печать размера core-файла
                print_core_size();
                break;
            case 'C':  // Установка размера core-файла
                set_core_size(option_args[i]);
                break;
            case 'd':  // Печать текущей директории
                print_current_directory();
                break;
            case 'v':  // Печать переменных окружения
                print_environment();
                break;
            case 'V':  // Установка переменной окружения
                set_environment_variable(option_args[i]);
                break;
            case 'h':  // Помощь
                printf("Usage: %s [options]\n", argv[0]);
                printf("Options:\n");
                printf("  -i  Print user and group IDs\n");
                printf("  -s  Become session leader\n");
                printf("  -p  Print process IDs\n");
                printf("  -u  Print ulimit\n");
                printf("  -U  Set ulimit\n");
                printf("  -c  Print core file size\n");
                printf("  -c  Set core file size\n");
                printf("  -d  Print current directory\n");
                printf("  -v  Print environment variables\n");
                printf("  -V  Set environment variable\n");
                printf("  -h  Show this help message\n");
                free(option_stack);
                free(option_args);
                return 0;
        }
    }
    
    free(option_stack);
    free(option_args);
    return 0;  // Успешное завершение программы
}