// Измените программу так, чтобы пользователю отводилось 5 секунд на ввод номера строки. Если пользователь не успевает, программа должна распечатать все содержимое файла и завершиться. Если же пользователь успел в течение пяти секунд ввести номер строки, то программа должна работать как в предыдущей задаче.

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

typedef struct {
    off_t offset;
    off_t length;
} Line;

typedef struct {
    Line* array;
    int cnt;
    int cap;
} Array;

void initArray(Array* a) {
    a->array = malloc(sizeof(Line));
    a->cnt = 0;
    a->cap = 1;
}

void insertArray(Array* a, Line element) {
    if (a->cnt == a->cap) {
        a->cap *= 2;
        a->array = realloc(a->array, a->cap * sizeof(Line));
    }

    a->array[a->cnt++] = element;
}

void freeArray(Array* a) {
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

// Global variables for timeout handling
static int timeout_occurred = 0;
static int fd_global = -1;
static Array* table_global = NULL;

// Signal handler for alarm
void timeout_handler(int sig) {
    if (sig == SIGALRM) {
        timeout_occurred = 1;
        printf("\nTimeout! 5 seconds elapsed. Printing entire file:\n");
        printf("==========================================\n");
        
        // Print entire file content
        lseek(fd_global, 0, SEEK_SET);
        char buffer[1024];
        int bytes_read;
        
        while ((bytes_read = read(fd_global, buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        printf("\n==========================================\n");
        printf("Program finished due to timeout.\n");
        
        // Clean up and exit
        close(fd_global);
        freeArray(table_global);
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) { 
        return 1; 
    }
    char* path = argv[1];

    Array table;
    initArray(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 1; 
    }
    
    // Set up global variables for signal handler
    fd_global = fd;
    table_global = &table;
    
    // Set up signal handler for alarm
    signal(SIGALRM, timeout_handler);
    // Get current position as mentioned in hint
    off_t current_pos = lseek(fd, 0L, SEEK_CUR);
    printf("Starting file analysis at position: %ld\n", current_pos);

    char c;
    off_t lineOffset = 0; //Offset of the line in the file
    off_t lineLength = 0; 
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            Line current = { lineOffset, lineLength };
            insertArray(&table, current);

            lineOffset += lineLength + 1;
            lineLength = 0;
        }
        else {
            lineLength++;
        }
    }

    if (lineLength > 0) {
        Line current = { lineOffset, lineLength };
        insertArray(&table, current);
    }

    // Print debugging table as mentioned in comments
    printf("\nLine Table (for debugging):\n");
    printf(" Line | Offset | Length\n");
    printf("------|--------|-------\n");
    for (int i = 0; i < table.cnt; i++) {
        printf("%5d | %6ld | %6ld\n", i + 1, table.array[i].offset, table.array[i].length);
    }
    printf("\nTotal lines: %d\n\n", table.cnt);

    // Set alarm for 5 seconds
    printf("You have 5 seconds to enter a line number. If no input, entire file will be printed.\n");
    alarm(5);
    
    while (1) {
        int num;
        printf("Enter the line number: ");
        fflush(stdout);
        
        // Check if timeout occurred
        if (timeout_occurred) {
            break;
        }
        
        if (scanf("%d", &num) != 1) {
            // Clear input buffer on error
            while (getchar() != '\n');
            continue;
        }
        
        // Cancel alarm since user provided input
        alarm(0);

        if (num == 0) { break; }
        if (table.cnt < num) {
            printf("The file contains only %d line(s).\n", table.cnt);
            // Reset alarm for next input
            alarm(5);
            continue;
        }

        Line line = table.array[num - 1]; //Line
        char* buf = calloc(line.length + 1, sizeof(char)); //Buffer

        if (lseek(fd, line.offset, SEEK_SET) == -1) {
            perror("Error seeking in file");
            free(buf);
            // Reset alarm for next input
            alarm(5);
            continue;
        }
        
        if (read(fd, buf, line.length) == -1) {
            perror("Error reading line");
            free(buf);
            // Reset alarm for next input
            alarm(5);
            continue;
        }

        printf("Line %d: %s\n", num, buf);
        free(buf);
        
        // Reset alarm for next input
        alarm(5);
    }

    close(fd);
    freeArray(&table);

    return 0;
}