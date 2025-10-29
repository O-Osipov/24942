#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __sun
#include <sys/fcntl.h>
#endif

#define EDITOR "nano"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <filename> <0|1> (0-допустимая, 1-обязательная)\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    int mandatory = atoi(argv[2]);
    
    printf("=== ЗАЩИЩЕННЫЙ РЕДАКТОР ===\n");
    printf("Режим блокировки: %s\n", mandatory ? "ОБЯЗАТЕЛЬНАЯ" : "ДОПУСТИМАЯ");
    printf("PID: %d\n", getpid());

    // 1. Открываем файл
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }

    // 2. Устанавливаем права для mandatory locking
    if (mandatory) {
        if (fchmod(fd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) | S_ISGID) == -1) {
            perror("Ошибка установки mandatory режима");
        }
        printf("Установлен S_ISGID бит для обязательной блокировки\n");
    }

    // 3. Устанавливаем блокировку на весь файл
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    printf("Устанавливаем блокировку на файл...\n");
    
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        printf("Не удалось установить блокировку: файл уже заблокирован\n");
        
        if (mandatory) {
            printf("❌ ОБЯЗАТЕЛЬНАЯ БЛОКИРОВКА: Редактор не будет запущен!\n");
            close(fd);
            exit(0);
        } else {
            printf("⚠ ДОПУСТИМАЯ БЛОКИРОВКА: Блокировка не установлена, но редактор запустится\n");
        }
    } else {
        printf("✓ Блокировка успешно установлена\n");
    }

    // 4. Запускаем редактор
    printf("Запуск редактора '%s'...\n", EDITOR);
    printf("=== ФАЙЛ ЗАБЛОКИРОВАН ===\n");
    
    char command[256];
    snprintf(command, sizeof(command), "%s %s", EDITOR, filename);
    int result = system(command);
    
    if (result == -1) {
        perror("Ошибка запуска редактора");
    }

    // 5. Закрываем файл (блокировка снимается)
    close(fd);
    printf("Блокировка снята. Завершение работы.\n");
    
    return 0;
}