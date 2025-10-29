#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

char* getbase(char* path) {
    char* base = strrchr(path, '/');
    return base ? base + 1 : path;
}

char gettype(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISREG(mode)) return '-';
    return '?';
}

void format_date(time_t mtime, char* buffer, size_t size) {
    struct tm* tm_info = localtime(&mtime);
    time_t now = time(NULL);
    struct tm* now_info = localtime(&now);
    
    if (now_info->tm_year - tm_info->tm_year > 0 || 
        (now_info->tm_year == tm_info->tm_year && now_info->tm_mon - tm_info->tm_mon > 6)) {
        strftime(buffer, size, "%b %d  %Y", tm_info);
    } else {
        strftime(buffer, size, "%b %d %H:%M", tm_info);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        struct stat sb;
        
        if (lstat(argv[i], &sb) == -1) {
            printf("Cannot access '%s'\n", argv[i]);s
            continue;
        }
        
        printf("%c", gettype(sb.st_mode));
        
        printf("%c", sb.st_mode & S_IRUSR ? 'r' : '-');
        printf("%c", sb.st_mode & S_IWUSR ? 'w' : '-');
        printf("%c", sb.st_mode & S_IXUSR ? 'x' : '-');
        printf("%c", sb.st_mode & S_IRGRP ? 'r' : '-');
        printf("%c", sb.st_mode & S_IWGRP ? 'w' : '-');
        printf("%c", sb.st_mode & S_IXGRP ? 'x' : '-');
        printf("%c", sb.st_mode & S_IROTH ? 'r' : '-');
        printf("%c", sb.st_mode & S_IWOTH ? 'w' : '-');
        printf("%c", sb.st_mode & S_IXOTH ? 'x' : '-');
        
        printf(" %3ld", (long)sb.st_nlink);
        
        struct passwd* pwd = getpwuid(sb.st_uid);
        struct group* grp = getgrgid(sb.st_gid);
        printf(" %-8s %-8s", 
               pwd ? pwd->pw_name : "?", 
               grp ? grp->gr_name : "?");
        
        if (S_ISREG(sb.st_mode)) {
            printf(" %8ld", (long)sb.st_size);
        } else {
            printf("         "); 
        }
        
        char date_buf[32];
        format_date(sb.st_mtime, date_buf, sizeof(date_buf));
        printf(" %s", date_buf);
        
        printf(" %s\n", getbase(argv[i]));
    }
    
    return 0;
}