#include <stdio.h>
#include <stdlib.h> //для exit
#include <unistd.h> //read, lseek, close, alarm
#include <fcntl.h>  //open, O_RDONLY
#include <sys/types.h>  
#include <sys/stat.h>
#include <signal.h> //для signal
#include <sys/mman.h> // для mman, munmap
#include <string.h> // для memchr

#define MAX_LINES 10000 //макс кол-во строк в файле
#define MAX_LINE_LENGTH 256 //макс длина одной строки
#define TIMEOUT 5 // время ожидания ввода в секундах

//структура для хранения инфы о строках
typedef struct{
    long offset; //смещение в байтах начала строки до новой строки 
    int length; // длина строки(без \n)
} LineInfo;

volatile sig_atomic_t timeout_occurred = 0;
int global_fd = -1;
LineInfo *global_lines = NULL;
int global_line_count = 0;

//обработка сигнала ALARM 
void alarm_handler(int sig){
    timeout_occurred = 1;
    printf("\n\nВремя на ввод истекло! Выводим содержимое файла: \n");
}

//фун-ия для вывода всего содержимого файла 
void print_entire_file(){
    if (global_fd == -1) return;

    //телепорт в начало файла
    lseek(global_fd, 0L, SEEK_SET);
    char buffer[1024];
    int bytes_read; 

    printf("_____ПОЛНОЕ СОДЕРЖИМОЕ ФАЙЛА_____\n");
    while ((bytes_read = read(global_fd, buffer, sizeof(buffer) - 1)) > 0){
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    printf("\n_____КОНЕЦ ФАЙЛА_____\n");
}

int main(int argc, char *argv[]){
    int fd; //файловый дескриптор
    char ch; //для чтения по одному символу
    LineInfo lines[MAX_LINES];
    int line_count = 0; //счетчик строк
    long current_offset = 0; //текущая позиция в файле(в байтах)
    int line_length = 0; //длина текущей обрабатываемой строки 
    int line_number; //номер строки для запроса 

    //проверяем аргументы командной строки 
    if (argc != 2){
        printf("Использование: %s <filename>\n", argv[0]);
        exit(1);
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("Ошибка открытия файла");
        exit(1);
    }

    //Инициализация глобальных переменных после открытия файла
    global_fd = fd;
    global_lines = lines;
    
    printf("Файл '%s' успешно открыт\n", argv[1]);

    //Шаг 1: Построение таблицы смещений и длин строк
    printf("\n===ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК===\n");
    lines[0].offset = 0; //первая строка начинается с позиции 0
    
    // Сохраняем начальную позицию
    current_offset = lseek(fd, 0L, SEEK_CUR);
    
    //читаем файл по одному символу
    while (read(fd, &ch, 1) > 0){
        line_length++; 

        //если встретили символ новой строки 
        if (ch == '\n'){
            //сохраняем информацию о текущей строке 
            lines[line_count].length = line_length - 1; //-1 чтобы исключить \n
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

    //обрабатываем последнюю строчку, если файл не заканчивается на \n
    if (line_length > 0 && line_count < MAX_LINES){
        lines[line_count].length = line_length;
        printf("Строка %d: смещение = %ld, длина = %d\n",
            line_count + 1, lines[line_count].offset, lines[line_count].length);
        line_count++;
    } 
    
    // Обновляем глобальный счетчик строк
    global_line_count = line_count;
    
    printf("\nВсего строк в файле: %d\n", line_count);
    
    //Шаг 2: Интерактивный запрос строк с ограничением времени
    printf("\n===Интерактивный режим===\n");
    printf("У вас есть %d секунд чтобы ввести номер строки\n", TIMEOUT);
    printf("Введите номер строки (1-%d) или 0 для выхода:\n", line_count);

    // Устанавливаем обработчик сигнала ALARM
    signal(SIGALRM, alarm_handler);
    alarm(TIMEOUT);    
    while(1){
        timeout_occurred = 0;
        
        // Устанавливаем будильник на 5 секунд
 
        
        printf("> ");
        fflush(stdout); // важно для вывода промпта
        
        if (scanf("%d", &line_number) != 1) {
            // Проверяем, не сработал ли таймаут
            if (timeout_occurred) {
                print_entire_file();
                break;
            }
            
            // Очищаем буфер ввода в случае ошибки
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Ошибка ввода. Введите число.\n");
            alarm(0); // сбрасываем будильник
            continue;
        }
        
        // Сбрасываем будильник после успешного ввода
        alarm(0);

        // Проверяем таймаут после ввода
        if (timeout_occurred) {
            print_entire_file();
            break;
        }

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

        //получаем информацию о запрошенной строке 
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
        char buffer[MAX_LINE_LENGTH + 1]; // +1 для нулевого терминатора
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
