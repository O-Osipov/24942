/*
    Программма для индексации и произвольного доступа к строкам файла
*/
#include <stdio.h>
#include <stdlib.h> //для exit
#include <unistd.h> //read, lseek, close
#include <fcntl.h>  //open, O_RDONLY
#include <sys/types.h>  
#include <sys/stat.h>

#define MAX_LINES 10000 //макс кол-во строк в файле
#define MAX_LINE_LENGTH 256 //макс длина одной строки

//структура для хранения информации о строках
typedef struct{
    long offset; //смещение(в байтах) начала строки в файле до начала новой строки 
    int length;  //длина строки(без символа новой строки)
} LineInfo;

int main(int argc, char *argv[]){
    int fd;   //файловый дескриптор (целое число, идентификатор открытого файла)
    char ch;  //для чтения по одному символу
    LineInfo lines[MAX_LINES]; // таблица информации о строках
    int line_count = 0;        // счетчик строк, тех которые уже обработаны
    long current_offset = 0;   // текущая позиция в файле(в байтах)
    int line_length = 0;       // длина текущей (обрабатываемой)строки 
    int line_number;           // номер строки для запроса, то есть от пользователя


    // проверяем аргументы командной строки
    if (argc != 2){
        printf("Использование: %s <filename>\n", argv[0]);
        exit(1);
    }
    fd = open(argv[1], O_RDONLY);//если успешно открыл, то возвращает неотрицательное число 
    //O_RDONLY - означает "Открыть только для чтения"
    if (fd == -1){
        perror("Ошибка открытия файла");
        exit(1);
    }
    
    printf("Файл '%s' успешно открыт\n", argv[1]);

    //Шаг 1: Построение таблицы смещений и длин строк
    printf("\n===ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК===\n");
    lines[0].offset = 0; //первая строка начинаеся с поз-и 0
    //читаем файл по одному символу
    while (read(fd, &ch, 1) > 0){
        line_length ++; 

        //если встретили символ новой строки 
        if (ch == '\n'){
            //сохраняем информацию о текущей строке 
            lines[line_count].length = line_length - 1; //-1 чтобы исключить /n
            printf("Строка %d: смещение = %ld, длина = %d\n", 
            line_count + 1, lines[line_count].offset, lines[line_count].length);

            line_count++;

            //получаем текущую позицию для начала следующей строки 
            current_offset = lseek(fd, 0L, SEEK_CUR);

            //защита от переполнения 
            if (line_count < MAX_LINES){
                lines[line_count].offset = current_offset;
            }
            line_length = 0; //сбрасываем счетчик длины для новой строки 
        }
    }

    //обраабатываем последнюю строчку, если файл не заканчивается на \n
    if (line_length > 0 && line_count < MAX_LINES){
        lines[line_count].length = line_length;
        printf("Строка %d: смещение = %ld, длина = %d\n",
        line_count + 1, lines[line_count].offset, lines[line_count].length);
        line_count++;
    } 
    printf("\nВсего строк в файле: %d\n", line_count);
    
    //Шаг 2: Интерактивный запрос строк 
    printf("\n===Интерактивный режим===\n");
    printf("Введите номер строки (1-%d) или 0 для выхода:\n", line_count);

    while(1){
        printf(">");
        scanf("%d", &line_number);

        //выход из программы 
        if (line_number == 0){
            printf("Выход из программы\n");
            break;
        }

        //проверка корректности номера строки
        if (line_number < 1 || line_number > line_count){
            printf("Ошибка - номер строки должен быть от 1 до %d\n", line_count);
            continue;
        }

        //получим информацию о запрошенной строке 
        int index = line_number - 1; //индексация с 0
        long offset = lines[index].offset;
        int length = lines[index].length;

        printf("Строка %d: смещение = %ld, длина = %d\n", line_number, offset, length);
        printf("Содержимое: ");

        //перемещаемся к началу строки 
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("ошибка позиционирования");
            continue;
        }

        //читаем и выводим строку
        char buffer[MAX_LINE_LENGTH];
        int bytes_read = read(fd, buffer, length);

        if (bytes_read > 0){
            //добавляем нулевой терминатор 
            buffer[bytes_read] = '\0';
            printf("'%s'\n", buffer);
        } else {
            printf("Ошибка чтения строки\n");
        }
    }
    //закрываем файл
    close(fd);
    return 0; 
}