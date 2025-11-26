#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>   // basename

#define TIME_BUF_SIZE 64

// Строим строку вида d rwxrwxrwx (10 символов: тип + 9 прав)
void build_mode_string(mode_t mode, char *buf) {
    // Тип файла
    if (S_ISDIR(mode)) {
        buf[0] = 'd';
    } else if (S_ISREG(mode)) {
        buf[0] = '-';
    } else {
        buf[0] = '?';
    }

    // Права: пользователь
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';

    // Права: группа
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';

    // Права: остальные
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';

    buf[10] = '\0';
}

// Возвращает только имя файла (без пути).
// basename(3) может модифицировать строку, поэтому делаем копию.
const char *get_filename_only(const char *path) {
    static char name_buf[1024];
    strncpy(name_buf, path, sizeof(name_buf) - 1);
    name_buf[sizeof(name_buf) - 1] = '\0';
    return basename(name_buf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILE...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        const char *path = argv[i];
        struct stat st;

        // lstat, чтобы символические ссылки обрабатывались как объект, а не как цель
        if (lstat(path, &st) == -1) {
            perror(path);
            continue;
        }

        char mode_str[11];
        build_mode_string(st.st_mode, mode_str);

        // Число жёстких ссылок
        long long links = (long long)st.st_nlink;

        // Владелец
        struct passwd *pw = getpwuid(st.st_uid);
        const char *user = pw ? pw->pw_name : "?";

        // Группа
        struct group *gr = getgrgid(st.st_gid);
        const char *group = gr ? gr->gr_name : "?";

        // Размер: только для обычного файла, иначе поле пустое
        char size_buf[32];
        if (S_ISREG(st.st_mode)) {
            snprintf(size_buf, sizeof(size_buf), "%lld", (long long)st.st_size);
        } else {
            size_buf[0] = '\0';
        }

        // Время модификации
        char time_buf[TIME_BUF_SIZE];
        struct tm *tm_info = localtime(&st.st_mtime);
        if (tm_info) {
            // Формат можно поменять по вкусу
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm_info);
        } else {
            strncpy(time_buf, "????????????????", sizeof(time_buf));
            time_buf[sizeof(time_buf) - 1] = '\0';
        }

        // Имя файла без пути
        const char *fname = get_filename_only(path);

        // Печать в виде таблицы с фиксированной шириной полей
        // mode(10) links(4) user(10) group(10) size(10) time(16) name
        printf("%-10s %4lld %-10s %-10s %10s %-16s %s\n",
               mode_str,
               links,
               user,
               group,
               size_buf,       // если пустое, printf напечатает просто пробелы
               time_buf,
               fname);
    }

    return 0;
}

