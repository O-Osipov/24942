#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct 
{
    off_t offset; // Позиция в файле где НАЧИНАЕТСЯ строка (в байтах от начала)
    off_t length; // Сколько байт ЗАНИМАЕТ строка (без символа \n)
} Line;

typedef struct 
{
    Line *array;
    int cnt;
    int cap;
} Array;

// Глобальные переменные для обработки сигнала
volatile sig_atomic_t timeout_occurred = 0; //флаг таймаута
char *file_data = NULL;    // Указатель на отображенный файл
size_t file_size = 0;      // Размер файла

// Обработчик сигнала ALARM
void alarm_handler(int sig) 
{
    timeout_occurred = 1; //устанавливаем флаг таймаута
}

void initArray(Array *a) 
{
    a->array = malloc(sizeof(Line));
    a->cnt = 0;
    a->cap = 1;
}

void insertArray(Array *a, Line element) 
{
    if (a->cnt == a->cap) 
    {
        a->cap *= 2 + 1;
        a->array = realloc(a->array, a->cap * sizeof(Line));
    }
    a->array[a->cnt++] = element;
}

void freeArray(Array *a) 
{
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

// Функция для вывода всего файла (использует mmap)
void printEntireFile() 
{
    printf("\n=== Timeout! Printing entire file: ===\n");
    if (file_data != NULL) 
    {
        // Просто выводим все содержимое файла напрямую из памяти
        fwrite(file_data, 1, file_size, stdout);
    }
    printf("\n");
}

// Построение таблицы строк с использованием mmap
Array buildLineTable(char *data, size_t size) 
{
    Array table;
    initArray(&table);
    
    off_t lineOffset = 0; //смещение начала текущей строки
    off_t lineLength = 0; // Длина текущей строки
    
    //Проходим по всем байтам файла в памяти
    for (size_t i = 0; i < size; i++) 
    {
        if (data[i] == '\n') //Найден конец строки - сохраняем информацию
        {
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);
            
            lineOffset = i + 1;  // Следующая строка начинается после \n
            lineLength = 0;
        } 
        else 
        {
            lineLength++;
        }
    }
    
    // Обработка последней строки, если файл не заканчивается \n
    if (lineLength > 0) 
    {
        Line current = {lineOffset, lineLength};
        insertArray(&table, current);
    }
    
    return table;
}

// Вывод строки по номеру (использует mmap)
void printLine(Array *table, int lineNumber) 
{
    if (table->cnt < lineNumber) 
    {
        printf("The file contains only %d line(s).\n", table->cnt);
        return;
    }
    
    Line line = table->array[lineNumber - 1];
    
    // Выводим строку напрямую из отображенной памяти
    printf("%.*s\n", (int)line.length, file_data + line.offset);
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    { 
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; 
    }
    
    // Открываем файл
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    { 
        perror("Failed to open file");
        return 1; 
    }
    
    // Получаем размер файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) 
    {
        perror("fstat failed");
        close(fd);
        return 1;
    }
    file_size = sb.st_size;
    
    // Отображаем файл в память
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //NULL - пусть система сама выбирает адрес
    //file_size - размер отображаемой области
    //PROT_READ - только чтение
    //MAP_PRIVATE - изменения не записываются в файл
    //fd - файловый дескриптор
    // 0 - смещение от начала файла
    if (file_data == MAP_FAILED) 
    {
        perror("mmap failed");
        close(fd);
        return 1;
    }
    
    // Файл можно закрыть - mmap сохраняет доступ
    close(fd);
    
    // Настройка обработчика сигнала ALARM
    signal(SIGALRM, alarm_handler);
    
    // Построение таблицы строк
    Array table = buildLineTable(file_data, file_size);
    printf("Loaded %d lines from file.\n", table.cnt);
    printf("You have 5 seconds to enter line number...\n");
    
    // Устанавливаем таймер на 5 секунд
    alarm(5);
    
    // Основной цикл
    while (!timeout_occurred) 
    {
        int num;
        printf("Enter the line number (0 to exit): ");
        
        // Проверяем, что scanf выполнился успешно и не было таймаута
        if (scanf("%d", &num) == 1) 
        {
            // Сбрасываем таймер при успешном вводе
            alarm(0);
            
            if (num == 0) break;
            printLine(&table, num);
            
            // Устанавливаем таймер снова для следующего ввода
            printf("You have 5 seconds for next input...\n");
            alarm(5);
        } 
        else 
        {
            // Если ввод некорректен, очищаем буфер ввода
            while (getchar() != '\n');
        }
    }
    
    // Если сработал таймаут, выводим весь файл
    if (timeout_occurred) 
    {
        printEntireFile();
    }
    
    // Освобождаем ресурсы
    if (file_data != NULL && file_data != MAP_FAILED) 
    {
        munmap(file_data, file_size);
    }
    freeArray(&table);
    
    return 0;
}
