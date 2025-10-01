// Написать программу, которая анализирует текстовый файл, созданный текстовым редактором, таким как ed(1) или vi(1). После запроса, который предлагает ввести номер строки, с использованием printf(3) программа печатает соответствующую строку текста. Ввод нулевого номера завершает работу программы. Используйте open(2), read(2), lseek(2) и close(2) для ввода/вывода. Постройте таблицу отступов в файле и длин строк для каждой строки файла. Как только эта таблица построена, позиционируйтесь на начало заданной строки и прочтите точную длину строки. 
// Подсказка: Выберите или создайте текстовый файл с короткими строками. Помните, что первая строка начинается с нулевого отступа в файле. Найдите каждый символ перевода строки, запишите его позицию; в программе следует использовать вызов lseek(fd, 0L, 1). Для отладки распечатайте эту таблицу и сравните с таблицей, полученной вручную. Как только таблицы начнут совпадать, можно приступать к запросу номера строки.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
    int fd;
    char *filename = "file.txt";
    char *buffer = NULL;
    char **lines = NULL;
    int line_count = 0;
    int line_number = 0;
    int bytes_read;
    int total_size = 0;
    int current_size = 1024;
    
    // Allocate initial buffer
    buffer = malloc(current_size);
    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror("open");
        free(buffer);
        return 1;
    }

    // Read entire file into buffer
    while ((bytes_read = read(fd, buffer + total_size, current_size - total_size)) > 0) {
        total_size += bytes_read;
        if (total_size >= current_size) {
            current_size *= 2;
            buffer = realloc(buffer, current_size);
            if (buffer == NULL) {
                perror("realloc");
                close(fd);
                return 1;
            }
        }
    }
    
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        free(buffer);
        return 1;
    }
    
    buffer[total_size] = '\0';  // Null terminate the string
    
    // Count lines
    for (int i = 0; i < total_size; i++) {
        if (buffer[i] == '\n') {
            line_count++;
        }
    }
    if (total_size > 0 && buffer[total_size-1] != '\n') {
        line_count++;  // Last line doesn't end with newline
    }
    
    // Allocate array for line pointers
    lines = malloc(line_count * sizeof(char*));
    if (lines == NULL) {
        perror("malloc");
        close(fd);
        free(buffer);
        return 1;
    }
    
    // Parse lines
    int line_idx = 0;
    char *line_start = buffer;
    for (int i = 0; i <= total_size; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            buffer[i] = '\0';  // Replace newline with null terminator
            lines[line_idx] = line_start;
            line_idx++;
            line_start = &buffer[i + 1];
        }
    }
    
    printf("File has %d lines\n", line_count);
    
    // Main loop for line number input
    while (1){
        printf("Enter line number (1-%d, 0 to exit): ", line_count);
        scanf("%d", &line_number);
        
        if (line_number == 0){
            break;
        }
        
        if (line_number < 1 || line_number > line_count) {
            printf("Invalid line number! Please enter a number between 1 and %d\n", line_count);
            continue;
        }
        
        printf("Line %d: %s\n", line_number, lines[line_number - 1]);
    }

    close(fd);
    free(buffer);
    free(lines);
    return 0;
}