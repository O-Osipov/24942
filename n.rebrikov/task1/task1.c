#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

void print_ids(); // Печать идентификаторов пользователя и группы
void become_leader_group(); // Стать лидером группы
void print_procces_id(); // Печатает идентификаторы процесса
void print_ulimit(); // Печатает значение ulimit
void set_ulimit(const char*); // Изменяет значение ulimit
void print_core_size(); // Печатает размер в байтах core-файла
void set_core_size(const char*); // Изменяет размер core-файла
void print_cwd(); // Печатает текущую рабочую директорию
void print_env(char *envp[]); // Распечатывает переменные среды и их значения
void set_env_var(const char*); // Вносит новую переменную в среду или изменяет значение существующей переменной


int main(int argc, char* argv[], char *envp[])
{
    int opt;
    while( (opt = getopt(argc,argv, "ispuU:cC:dvV:")) != -1)
    {
        switch(opt)
        {
            case 'i': print_ids(); break;
            case 's': become_leader_group(); break;
            case 'p': print_procces_id(); break;
            case 'u': print_ulimit(); break;
            case 'U': set_ulimit(optarg); break;
            case 'c': print_core_size(); break;
            case 'C': set_core_size(optarg); break;
            case 'd': print_cwd(); break;
            case 'v': print_env(envp); break;
            case 'V': set_env_var(optarg); break;
            default: fprintf(stderr, "Unknown option: -%c\n", optopt);
		fprintf(stderr, "Usage: task1 [-ispuU:cC:dvV:]\n");
		exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void print_ids()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("Real GID: %d\n", getgid());
    printf("Effective GID: %d\n", getegid());
}

void become_leader_group()
{
    if(setpgid(0,0) == -1)
    {
        perror("setpgid failed");
    }
}

void print_procces_id()
{
    printf("Procces ID: %d\n", getpid());
    printf("Parent Procces ID: %d\n", getppid());
    printf("Procces Group ID: %d\n", getpgrp());
}

void print_ulimit()
{
    long max_processes = sysconf(_SC_CHILD_MAX);
    if (max_processes != -1) {
        printf("Max child processes per user: %ld\n", max_processes);
    } else {
        perror("sysconf failed");
    }
}

void set_ulimit(const char* str)
{
    long new_size = strtol(str,NULL,10);
    if(errno == ERANGE)
    {
        fprintf(stderr, "Invalid ulimit value: %s\n", str);
        return;
    }
    struct rlimit rl;
    rl.rlim_cur = new_size;
    rl.rlim_max = new_size;

    if(setrlimit(RLIMIT_FSIZE, &rl) == -1)
    {
        perror("setrlimit failed");
    }
}

void print_core_size()
{
    struct rlimit rl;
    if(getrlimit(RLIMIT_CORE, &rl) == 0)
    {
        printf("Core file size: %ld bytes\n", (long)rl.rlim_cur);
    }
}

void set_core_size(const char* str)
{
    long new_size = strtol(str,NULL,10);
    if(errno = ERANGE)
    {
        fprintf(stderr, "Invalid core size: %s\n",str);
        return;
    }
    struct rlimit rl;
    rl.rlim_cur = new_size;
    rl.rlim_max = new_size;

    if(setrlimit(RLIMIT_CORE, &rl) == -1)
    {
        perror("setrlimit core failed");
    }
}

void print_cwd()
{
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd failed");
    }
}

void print_env(char *envp[])
{
    for(char** env = envp; (*env) != NULL; env++)
    {
        printf("%s\n", (*env));
    }
}

void set_env_var(const char* assignment)
{
    char* equals = strchr(assignment, '='); // возвращает указатель на первое вхождение '='
    if(!equals)
    {
        fprintf(stderr, "Invalid environment assignment: %s\n", assignment);
    }

    size_t name_len = equals - assignment;
    char name[name_len + 1];
    strncpy(name,assignment,name_len);
    name[name_len] = '\0';

    if(setenv(name,equals + 1,1) == -1)
    {
        perror("setenv failed");
    }
}
