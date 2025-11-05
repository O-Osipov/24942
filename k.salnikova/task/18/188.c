#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

// Функция для получения имени файла из пути
char* getbase(char* path) {
    char* base = strrchr(path, '/');  // Ищем последнее вхождение '/'
    return base ? base + 1 : path;    // Если нашли - возвращаем часть после '/', иначе весь путь
}

// Функция определения типа файла по его режиму доступа
char gettype(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';    // Каталог
    if (S_ISREG(mode)) return '-';    // Обычный файл
    if (S_ISLNK(mode)) return 'l';    // Символическая ссылка
    if (S_ISCHR(mode)) return 'c';    // Символьное устройство
    if (S_ISBLK(mode)) return 'b';    // Блочное устройство
    if (S_ISFIFO(mode)) return 'p';   // FIFO (именованный канал)
    if (S_ISSOCK(mode)) return 's';   // Сокет
    return '?';                       // Неизвестный тип
}

// Функция форматирования даты в стиле ls
void format_date_ls(time_t mtime, char* buffer, size_t size) {
    struct tm* tm_info = localtime(&mtime);  // Преобразуем время в локальную структуру
    time_t now = time(NULL);                 // Текущее время
    struct tm* now_info = localtime(&now);
    
    // Если файл старше 6 месяцев, показываем год вместо времени
    if (now_info->tm_year - tm_info->tm_year > 0 || 
        (now_info->tm_year == tm_info->tm_year && now_info->tm_mon - tm_info->tm_mon > 6)) {
        strftime(buffer, size, "%b %_d  %Y", tm_info);  // Формат: "Месяц День Год"
    } else {
        strftime(buffer, size, "%b %_d %H:%M", tm_info); // Формат: "Месяц День Время"
    }
}

int main(int argc, char *argv[]) {
    // Проверка количества аргументов
    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }
    
    // Устанавливаем локаль для корректного отображения (особенно дат)
    setlocale(LC_ALL, "");
    
    // Обрабатываем каждый переданный файл/каталог
    for (int i = 1; i < argc; i++) {
        struct stat sb;  // Структура для хранения информации о файле
        
        // Получаем информацию о файле (lstat работает и с симлинками)
        if (lstat(argv[i], &sb) == -1) {
            printf("Cannot access '%s'\n", argv[i]);  // Ошибка доступа
            continue;  // Переходим к следующему файлу
        }
        
        // 1. ТИП ФАЙЛА И ПРАВА ДОСТУПА
        // Выводим тип файла (d, -, l, и т.д.)
        printf("%c", gettype(sb.st_mode));
        
        // Права доступа для владельца (user)
        printf("%c%c%c", sb.st_mode & S_IRUSR ? 'r' : '-',  // Чтение
                         sb.st_mode & S_IWUSR ? 'w' : '-',  // Запись
                         sb.st_mode & S_IXUSR ? 'x' : '-'); // Исполнение
        
        // Права доступа для группы (group)
        printf("%c%c%c", sb.st_mode & S_IRGRP ? 'r' : '-',
                         sb.st_mode & S_IWGRP ? 'w' : '-',
                         sb.st_mode & S_IXGRP ? 'x' : '-');
        
        // Права доступа для остальных (others)
        printf("%c%c%c", sb.st_mode & S_IROTH ? 'r' : '-',
                         sb.st_mode & S_IWOTH ? 'w' : '-',
                         sb.st_mode & S_IXOTH ? 'x' : '-');
        
        // 2. КОЛИЧЕСТВО ССЫЛОК (hard links)
        printf(" %ld", (long)sb.st_nlink);
        
        // 3. ВЛАДЕЛЕЦ И ГРУППА
        struct passwd* pwd = getpwuid(sb.st_uid);  // Получаем имя владельца по UID
        struct group* grp = getgrgid(sb.st_gid);   // Получаем имя группы по GID
        printf(" %s %s", 
               pwd ? pwd->pw_name : "?",   // Имя владельца или "?" если не найдено
               grp ? grp->gr_name : "?");  // Имя группы или "?" если не найдено
        
        // 4  РАЗМЕР ФАЙЛА (в байтах)
        printf(" %ld", (long)sb.st_size);
        
        // 5. ДАТА ПОСЛЕДНЕЙ МОДИФИКАЦИИ
        char date_buf[32];
        format_date_ls(sb.st_mtime, date_buf, sizeof(date_buf));  // Форматируем дату
        printf(" %s", date_buf);
        
        // 6 ИМЯ ФАЙЛА (только базовое имя, без пути)
        printf(" %s\n", getbase(argv[i]));
    }
    
    return 0;
}