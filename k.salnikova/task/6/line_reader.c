#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024

struct LineInfo {
    off_t offset;
    size_t length;
};

int fd_global;
struct LineInfo *lines_global;
int line_count_global;
volatile sig_atomic_t timeout_occurred = 0;

void alarm_handler(int sig) {
    timeout_occurred = 1;
}

void print_entire_file(int fd, struct LineInfo *lines, int line_count) {
    printf("\n ВРЕМЯ ВЫШЛО! Вывод всего файла:\n");
    printf("========================================\n");
    
    char line_buffer[BUFFER_SIZE];
    
    for (int i = 0; i < line_count; i++) {
        if (lseek(fd, lines[i].offset, SEEK_SET) == (off_t)-1) {
            perror("Ошибка позиционирования");
            continue;
        }
        
        ssize_t read_bytes = read(fd, line_buffer, lines[i].length);
        if (read_bytes == -1) {
            perror("Ошибка чтения строки");
            continue;
        }
        
        line_buffer[read_bytes] = '\0';
        printf("%d: %s\n", i + 1, line_buffer);
    }
    printf("========================================\n");
}

int main(int argc, char *argv[]) {
    int fd;
    char ch;
    struct LineInfo lines[MAX_LINES];
    int line_count = 0;
    ssize_t bytes_read;
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }

    fd_global = fd;
    lines_global = lines;
    
    printf("=== АНАЛИЗ ФАЙЛА: %s ===\n", argv[1]);

    lines[0].offset = 0;
    off_t line_start = 0;
    off_t current_pos = lseek(fd, 0L, SEEK_CUR);
    
    while ((bytes_read = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            current_pos = lseek(fd, 0L, SEEK_CUR);
            lines[line_count].length = current_pos - 1 - line_start;
            
            line_count++;
            if (line_count >= MAX_LINES) break;
            
            line_start = current_pos;
            lines[line_count].offset = line_start;
        }
    }
    
    current_pos = lseek(fd, 0L, SEEK_CUR);
    if (current_pos > line_start && line_count < MAX_LINES) {
        lines[line_count].length = current_pos - line_start;
        line_count++;
    }
    
    line_count_global = line_count;
    
    printf("Найдено строк: %d\n", line_count);
    
    signal(SIGALRM, alarm_handler);
    
    printf("\n У вас 5 секунд чтобы ввести номер строки!\n");
    printf("Введите номер строки (1-%d), 0 для выхода:\n", line_count);
    
    int line_number;
    char line_buffer[BUFFER_SIZE];
    
    alarm(5);
    
    while (1) {
        printf("Введите номер строки: ");
        fflush(stdout);
        
        if (scanf("%d", &line_number) != 1) {
            if (timeout_occurred) {
                print_entire_file(fd, lines, line_count);
                break;
            }
            printf("Ошибка ввода.\n");
            while (getchar() != '\n');
            continue;
        }

        alarm(0);
        
        if (timeout_occurred) {
            print_entire_file(fd, lines, line_count);
            break;
        }
        
        if (line_number == 0) {
            printf("Завершение работы.\n");
            break;
        }
        
        if (line_number < 1 || line_number > line_count) {
            printf("Ошибка: номер строки должен быть от 1 до %d\n", line_count);
            alarm(5);
            continue;
        }
        
        int index = line_number - 1;
        
        if (lseek(fd, lines[index].offset, SEEK_SET) == (off_t)-1) {
            perror("Ошибка позиционирования");
            alarm(5);
            continue;
        }
        
        ssize_t read_bytes = read(fd, line_buffer, lines[index].length);
        if (read_bytes == -1) {
            perror("Ошибка чтения строки");
            alarm(5);
            continue;
        }
        
        line_buffer[read_bytes] = '\0';
        printf("Строка %d: %s\n", line_number, line_buffer);
        
        alarm(5);
    }
    
    close(fd);
    return 0;
}