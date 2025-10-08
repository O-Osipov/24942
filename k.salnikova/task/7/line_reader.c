#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024

struct LineInfo {
    off_t offset;
    size_t length;
};

char *file_data = NULL;
size_t file_size = 0;
struct LineInfo *lines_global;
int line_count_global;

void print_entire_file(char *data, struct LineInfo *lines, int line_count) {
    printf("\n ВРЕМЯ ВЫШЛО! Вывод всего файла:\n");
    printf("========================================\n");
    
    for (int i = 0; i < line_count; i++) {
        char *line_start = data + lines[i].offset;
        printf("%d: ", i + 1);
        for (size_t j = 0; j < lines[i].length; j++) {
            putchar(line_start[j]);
        }
        printf("\n");
    }
    printf("========================================\n");
}

int read_number_with_timeout(int timeout_seconds) {
    char buffer[100];
    int number = 0;
    int pos = 0;
    time_t start_time = time(NULL);
    
    printf("Введите номер строки: ");
    fflush(stdout);
    
    while (1) {
        if (time(NULL) - start_time >= timeout_seconds) {
            return -2;
        }
        
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        
        if (ready > 0) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) == 1) {
                if (ch == '\n' || ch == '\r') {
                    if (pos > 0) {
                        buffer[pos] = '\0';
                        number = atoi(buffer);
                        return number;
                    }
                } else if (ch >= '0' && ch <= '9' && pos < 99) {
                    buffer[pos++] = ch;
                    putchar(ch);
                    fflush(stdout);
                } else if (ch == 127 || ch == 8) {
                    if (pos > 0) {
                        pos--;
                        printf("\b \b");
                        fflush(stdout);
                    }
                }
            }
        } else if (ready == 0) {
            continue;
        } else {
            perror("select");
            return -1;
        }
    }
}

int main(int argc, char *argv[]) {
    int fd;
    struct LineInfo lines[MAX_LINES];
    int line_count = 0;
    struct stat file_stat;
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    if (fstat(fd, &file_stat) == -1) {
        perror("Ошибка получения информации о файле");
        close(fd);
        return 1;
    }
    
    file_size = file_stat.st_size;
    
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Ошибка отображения файла в память");
        close(fd);
        return 1;
    }
    
    printf("=== АНАЛИЗ ФАЙЛА: %s (размер: %ld байт) ===\n", argv[1], file_size);
    
    lines[0].offset = 0;
    size_t line_start = 0;
    
    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            lines[line_count].length = i - line_start;
            line_count++;
            
            if (line_count >= MAX_LINES) break;
            
            line_start = i + 1;
            lines[line_count].offset = line_start;
        }
    }
    
    if (line_count < MAX_LINES && line_start < file_size) {
        lines[line_count].length = file_size - line_start;
        line_count++;
    }
    
    lines_global = lines;
    line_count_global = line_count;
    
    printf("Найдено строк: %d\n", line_count);
    
    printf("\nТАБЛИЦА СТРОК:\n");
    printf("№\tСмещение\tДлина\n");
    printf("--\t--------\t-----\n");
    for (int i = 0; i < line_count; i++) {
        printf("%d\t%ld\t\t%zu\n", i + 1, (long)lines[i].offset, lines[i].length);
    }
    
    printf("\n У вас 5 секунд чтобы ввести номер строки!\n");
    printf("Введите номер строки (1-%d), 0 для выхода:\n", line_count);
    
    int line_number;
    
    while (1) {
        line_number = read_number_with_timeout(5);
        
        if (line_number == -2) {
            print_entire_file(file_data, lines, line_count);
            break;
        } else if (line_number == -1) {
            printf("Ошибка ввода.\n");
            continue;
        }
        
        printf("\n");
        
        if (line_number == 0) {
            printf("Завершение работы.\n");
            break;
        }
        
        if (line_number < 1 || line_number > line_count) {
            printf("Ошибка: номер строки должен быть от 1 до %d\n", line_count);
            continue;
        }
        
        int index = line_number - 1;
        char *line_start = file_data + lines[index].offset;
        size_t line_length = lines[index].length;
        
        printf("Строка %d: ", line_number);
        for (size_t i = 0; i < line_length; i++) {
            putchar(line_start[i]);
        }
        printf("\n");
    }
    
    if (file_data != NULL && file_data != MAP_FAILED) {
        munmap(file_data, file_size);
    }
    close(fd);
    
    return 0;
}