// Написать программу, которая анализирует текстовый файл, созданный текстовым редактором, таким как ed(1) или vi(1). После запроса, который предлагает ввести номер строки, с использованием printf(3) программа печатает соответствующую строку текста. Ввод нулевого номера завершает работу программы. Используйте open(2), read(2), lseek(2) и close(2) для ввода/вывода. Постройте таблицу отступов в файле и длин строк для каждой строки файла. Как только эта таблица построена, позиционируйтесь на начало заданной строки и прочтите точную длину строки. 
// Подсказка: Выберите или создайте текстовый файл с короткими строками. Помните, что первая строка начинается с нулевого отступа в файле. Найдите каждый символ перевода строки, запишите его позицию; в программе следует использовать вызов lseek(fd, 0L, 1). Для отладки распечатайте эту таблицу и сравните с таблицей, полученной вручную. Как только таблицы начнут совпадать, можно приступать к запросу номера строки.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
    int fd;

    char *filename = "file.txt";
    char buffer[1024];
    
    int *line_offsets = NULL;
    int *line_lengths = NULL;
    int line_count = 0, line_number = 0, line_idx = 0;
    int line_start_pos = 0, line_length = 0;
    int bytes_read;
    int current_pos = 0;
    
    
    fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror("open");
        return 1;
    }
    
    // Count lines
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                line_count++;
            }
        }
    }
    
    // Check if last line doesn't end with newline
    lseek(fd, -1, SEEK_END);
    read(fd, buffer, 1);
    if (buffer[0] != '\n') {
        line_count++;
    }
    
    line_offsets = malloc(line_count * sizeof(int));
    line_lengths = malloc(line_count * sizeof(int));
    
    if (line_offsets == NULL || line_lengths == NULL) {
        perror("malloc");
        close(fd);
        return 1;
    }
    
    // Reset to beginning and build offset table
    lseek(fd, 0, SEEK_SET);
    current_pos = 0;
    line_start_pos = 0;
    line_length = 0;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                line_offsets[line_idx] = line_start_pos;
                line_lengths[line_idx] = line_length;
                
                printf("Line %d: offset=%d, length=%d\n", 
                       line_idx + 1, line_start_pos, line_length);
                
                line_idx++;
                line_start_pos = current_pos + i + 1;
                line_length = 0;
            } else {
                line_length++;
            }
            current_pos++;
        }
    }

    if (line_idx < line_count) {
        line_offsets[line_idx] = line_start_pos;
        line_lengths[line_idx] = line_length;
        printf("Line %d: offset=%d, length=%d\n", 
               line_idx + 1, line_start_pos, line_length);
    }
    
    printf("\nFile has %d lines\n", line_count);
    
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
        
        // Position at the start of the requested line
        int target_line = line_number - 1;
        int offset = line_offsets[target_line];
        int length = line_lengths[target_line];
        
        printf("Positioning at offset %d, reading %d bytes\n", offset, length);
        
        // Use lseek to position at the line start
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }
        
        // Read exactly the line length
        bytes_read = read(fd, buffer, length);
        if (bytes_read == -1) {
            perror("read");
            continue;
        }
        
        // Null terminate and print
        buffer[bytes_read] = '\0';
        printf("Line %d: %s\n", line_number, buffer);
    }

    close(fd);
    free(line_offsets);
    free(line_lengths);
    return 0;
}