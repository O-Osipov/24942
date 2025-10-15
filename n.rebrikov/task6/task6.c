#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

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
volatile sig_atomic_t timeout_occurred = 0;
int fd_global; // Сохраняем файловый дескриптор чтобы использовать в обработчике сигнала

// Обработчик сигнала ALARM
void alarm_handler(int sig) 
{
    timeout_occurred = 1; // Флаг устанавливается при срабатывании таймера
}
// SIGALRM посылается когда срабатывает таймер alarm()

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
        a->cap *= 2;
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

// Функция для вывода всего файла
void printEntireFile(int fd) 
{
    printf("\n=== Timeout! Printing entire file: ===\n");
    
    lseek(fd, 0, SEEK_SET);  // Перемещаемся в начало файла
    // SEEK_SET - отсчет от начала файла
    
    char buffer[1024]; // Буфер на 1KB для чтения
    ssize_t bytes_read; // Сколько байт реально прочитано
    
    // Читаем файл блоками по 1024 байта пока не кончится
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) 
    {
        write(STDOUT_FILENO, buffer, bytes_read); // Выводим прочитанное на экран
        // STDOUT_FILENO = 1 (дескриптор стандартного вывода)
    }
    printf("\n");
}

Array buildLineTable(int fd) 
{
    Array table;
    initArray(&table);
    
    char c; // Буфер для одного символа
    off_t lineOffset = 0; // Текущая позиция начала строки в файле
    off_t lineLength = 0; // Длина текущей строки
    
    // Читаем файл по одному символу
    while (read(fd, &c, 1) == 1) // read возвращает 1 если прочитал символ
    {
        if (c == '\n') 
        {
            // Нашли конец строки - сохраняем информацию
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);
            
            // Следующая строка начинается после \n
            lineOffset += lineLength + 1; // +1 потому что пропускаем \n
            lineLength = 0; // Начинаем считать длину новой строки
        } 
        else 
        {
            lineLength++; // Увеличиваем длину текущей строки
        }
    }
    
    // Если последняя строка не заканчивается \n - сохраняем ее
    if (lineLength > 0) 
    {
        Line current = {lineOffset, lineLength};
        insertArray(&table, current);
    }
    
    return table;
}

void printLine(int fd, Array *table, int lineNumber) 
{
    if (table->cnt < lineNumber) // Проверяем что строка существует
    {
        printf("The file contains only %d line(s).\n", table->cnt);
        return;
    }
    
    // Берем информацию о строке (индексация с 0)
    Line line = table->array[lineNumber - 1];
    // Выделяем память под строку +1 для нулевого байта
    char *buf = calloc(line.length + 1, sizeof(char));
    
    lseek(fd, line.offset, SEEK_SET); // Перемещаемся к началу строки в файле
    read(fd, buf, line.length); // Читаем строку в буфер
    
    printf("%s\n", buf); //Выводим строку
    free(buf); //Освобождаем память
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    { 
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; 
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    { 
        perror("Failed to open file");
        return 1; 
    }
    fd_global = fd;  // Сохраняем для обработчика сигнала
    
    // Настройка обработчика сигнала ALARM
    signal(SIGALRM, alarm_handler);
    
    // Построение таблицы строк
    Array table = buildLineTable(fd);
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
            printLine(fd, &table, num);
            
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
        printEntireFile(fd);
    }
    
    close(fd);
    freeArray(&table);
    return 0;
}
