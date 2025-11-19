#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILENAME 256
#define MAX_USERNAME 32
#define MAX_GROUPNAME 32

void get_permissions(mode_t mode, char *perms) {
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISREG(mode)) perms[0] = '-';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    else perms[0] = '?';

    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
}

const char* get_basename(const char *path) {
    const char *basename = strrchr(path, '/');
    return basename ? basename + 1 : path;
}

void format_date(time_t mtime, char *date_str) {
    struct tm *timeinfo = localtime(&mtime);
    time_t now = time(NULL);
    
    if (difftime(now, mtime) > 15778476) {
        strftime(date_str, 13, "%b %e  %Y", timeinfo);
    } else {
        strftime(date_str, 13, "%b %e %H:%M", timeinfo);
    }
}

void print_file_info(const char *filename) {
    struct stat st;
    char perms[11];
    char date_str[13];
    const char *basename = get_basename(filename);
    
    if (lstat(filename, &st) == -1) {
        fprintf(stderr, "ls: невозможно получить доступ к '%s': ", filename);
        perror("");
        return;
    }
    
    get_permissions(st.st_mode, perms);
    format_date(st.st_mtime, date_str);
    
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    
    char user_name[MAX_USERNAME];
    char group_name[MAX_GROUPNAME];
    
    if (pw) {
        snprintf(user_name, MAX_USERNAME, "%s", pw->pw_name);
    } else {
        snprintf(user_name, MAX_USERNAME, "%d", st.st_uid);
    }
    
    if (gr) {
        snprintf(group_name, MAX_GROUPNAME, "%s", gr->gr_name);
    } else {
        snprintf(group_name, MAX_GROUPNAME, "%d", st.st_gid);
    }
    
    // Вывод размера файла для всех типов файлов
    printf("%s %2ld %-8s %-8s %8ld %s %s\n", 
           perms, 
           st.st_nlink,
           user_name,
           group_name,
           st.st_size,  // Всегда выводим размер
           date_str,
           basename);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл1> [файл2 ...]\n", argv[0]);
        fprintf(stderr, "Аналог команды: ls -ld\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }
    
    return 0;
}