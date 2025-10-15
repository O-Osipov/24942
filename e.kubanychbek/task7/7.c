/*
    Программа для индексации и произвольного доступа к строкам файла
    ВАРИАНТ С MMAP: файл отображается в память, чтение идёт из памяти,
    НЕТ использования read(2)/lseek(2)/write(2) для чтения содержимого файла.
*/

#define __EXTENSIONS__ /* ADDED: для Solaris — раскрывает расширения в заголовках */
#include <stdio.h>
#include <stdlib.h>     // exit
#include <unistd.h>     // close, alarm /* REMOVED: read, lseek из использования */
#include <fcntl.h>      // open, O_RDONLY
#include <sys/types.h>
#include <sys/stat.h>   // fstat
#include <signal.h>     // signal
#include <sys/mman.h>   // ADDED: mmap, munmap
#include <string.h>     // memchr
#include <errno.h>

#define MAX_LINES 10000         // макс кол-во строк в файле
#define MAX_LINE_LENGTH 256     // макс длина одной строки при печати (защита)
#define TIMEOUT 5               // время ожидания ввода в секундах

// структура для хранения инфы о строках
typedef struct{
    /* REMOVED: long offset;  -- long может быть узким
       ADDED: используем off_t (POSIX тип для смещений), корректно под 64-бит */
    off_t offset;    // смещение в байтах от начала файла
    int   length;    // длина строки (без '\n')
} LineInfo;

/* === ГЛОБАЛЬНАЯ СЕКЦИЯ ДЛЯ MMAP === */
volatile sig_atomic_t timeout_occurred = 0;

/* REMOVED:
int global_fd = -1;
LineInfo *global_lines = NULL;
int global_line_count = 0;
*/

/* ADDED: базовый адрес отображённого файла и размер */
static const char *g_base = NULL;  // указатель на начало отображенного файла (read-only)
static size_t      g_size = 0;     // размер файла (в байтах)
static LineInfo    g_lines[MAX_LINES]; // индекс строк здесь
static int         g_line_count = 0;   // число строк

// обработка сигнала ALARM
void alarm_handler(int sig){
    (void)sig;
    timeout_occurred = 1;
    printf("\n\nВремя на ввод истекло! Выводим содержимое файла: \n");
}

/* фун-ия для вывода всего содержимого файла */
/* REMOVED: старая версия читала через lseek/read буферами
void print_entire_file(){
    if (global_fd == -1) return;

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
*/

/* ADDED: версия печати всего файла напрямую из отображения */
void print_entire_file_mmap(void){
    if (!g_base || g_size == 0) return;
    printf("_____ПОЛНОЕ СОДЕРЖИМОЕ ФАЙЛА_____\n");
    /* Печатаем ровно g_size байт, независимо от наличия '\0' */
    printf("%.*s", (int)g_size, g_base);
    printf("\n_____КОНЕЦ ФАЙЛА_____\n");
}

/* ADDED: построение индекса строк по отображению
   Идея: сканируем [g_base, g_base + g_size), находим '\n' через memchr,
   для каждой строки фиксируем offset (от начала файла) и length (без '\n'). */
int build_index_from_mmap(const char *base, size_t size, LineInfo *lines, int max_lines){
    const char *p   = base;
    const char *end = base + size;
    int count = 0;

    if (size == 0) return 0;          // пустой файл — строк нет

    /* первая строка начинается с offset 0 */
    if (count < max_lines) lines[count].offset = (off_t)0;

    while (p < end){
        /* ищем перевод строки в оставшемся фрагменте */
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (nl){
            if (count < max_lines){
                /* длина = позиция \n минус начало текущей строки */
                lines[count].length = (int)(nl - (base + lines[count].offset));
                printf("Строка %d: смещение = %lld, длина = %d\n",
                       count + 1, (long long)lines[count].offset, lines[count].length);
            }
            count++;
            if (count >= max_lines) break;

            /* следующая строка начинается сразу после '\n' */
            lines[count].offset = (off_t)((nl + 1) - base);
            p = nl + 1;
        } else {
            /* последняя строка (без '\n' до конца файла) */
            if (count < max_lines){
                lines[count].length = (int)((end - base) - lines[count].offset);
                printf("Строка %d: смещение = %lld, длина = %d\n",
                       count + 1, (long long)lines[count].offset, lines[count].length);
            }
            count++;
            break;
        }
    }

    return count;
}

int main(int argc, char *argv[]){
    int fd; // файловый дескриптор
    /* REMOVED:
    char ch; // для чтения по одному символу
    LineInfo lines[MAX_LINES];
    int line_count = 0; // счетчик строк
    long current_offset = 0; // текущая позиция
    int line_length = 0; // длина текущей строки
    */
    int line_number; // номер строки для запроса 

    // проверяем аргументы командной строки 
    if (argc != 2){
        printf("Использование: %s <filename>\n", argv[0]);
        exit(1);
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("Ошибка открытия файла");
        exit(1);
    }

    /* REMOVED: глобальные переменные, завязанные на fd/lines
    global_fd = fd;
    global_lines = lines;
    */

    printf("Файл '%s' успешно открыт\n", argv[1]);

    /* ADDED: узнаём размер файла и отображаем его в память */
    struct stat st;
    if (fstat(fd, &st) == -1){
        perror("fstat");
        close(fd);
        exit(1);
    }
    if (!S_ISREG(st.st_mode)){
        fprintf(stderr, "Ошибка: '%s' не является обычным файлом\n", argv[1]);
        close(fd);
        exit(1);
    }
    g_size = (size_t)st.st_size;

    if (g_size == 0){
        printf("Пустой файл.\n");
        close(fd);
        return 0;
    }

    void *addr = mmap(NULL, g_size, PROT_READ, MAP_SHARED, fd, 0); // ADDED
    if (addr == MAP_FAILED){
        perror("mmap");
        close(fd);
        exit(1);
    }
    g_base = (const char *)addr;

    // Шаг 1: Построение таблицы смещений и длин строк
    printf("\n===ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК===\n");

    /* REMOVED: побайтное чтение через read + вычисление offset через lseek
    lines[0].offset = 0; 
    current_offset = lseek(fd, 0L, SEEK_CUR);
    while (read(fd, &ch, 1) > 0){
        line_length++; 
        if (ch == '\n'){
            lines[line_count].length = line_length - 1;
            printf("Строка %d: смещение = %ld, длина = %d\n", 
                line_count + 1, lines[line_count].offset, lines[line_count].length);
            line_count++;
            current_offset = lseek(fd, 0L, SEEK_CUR);
            if (line_count < MAX_LINES){
                lines[line_count].offset = current_offset;
            }
            line_length = 0;
        }
    }
    if (line_length > 0 && line_count < MAX_LINES){
        lines[line_count].length = line_length;
        printf("Строка %d: смещение = %ld, длина = %d\n",
            line_count + 1, lines[line_count].offset, lines[line_count].length);
        line_count++;
    } 
    */

    /* ADDED: индекс строим по содержимому mmap — без read/lseek */
    g_line_count = build_index_from_mmap(g_base, g_size, g_lines, MAX_LINES);
    
    printf("\nВсего строк в файле: %d\n", g_line_count);
    
    // Интерактивный запрос строк с ограничением времени
    printf("\n===Интерактивный режим===\n");
    printf("У вас есть %d секунд чтобы ввести номер строки\n", TIMEOUT);
    printf("Введите номер строки (1-%d) или 0 для выхода:\n", g_line_count);

    // Устанавливаем обработчик сигнала ALARM
    signal(SIGALRM, alarm_handler);
    alarm(TIMEOUT);

    while(1){
        timeout_occurred = 0;
        printf("> ");
        fflush(stdout); // важно для вывода промпта
        
        if (scanf("%d", &line_number) != 1) {
            if (timeout_occurred) {
                /* REMOVED: print_entire_file(); */
                /* ADDED: читаем из mmap */
                print_entire_file_mmap();
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

        if (timeout_occurred) {
            /* REMOVED: print_entire_file(); */
            /* ADDED: версия через mmap */
            print_entire_file_mmap();
            break;
        }

        // выход из программы 
        if (line_number == 0){
            printf("Выход из программы\n");
            break;
        }

        // проверка корректности номера строки
        if (line_number < 1 || line_number > g_line_count){
            printf("Ошибка - номер строки должен быть от 1 до %d\n", g_line_count);
            continue;
        }

        // получаем информацию о запрошенной строке
        int  index  = line_number - 1; // индексация с 0
        off_t offset = g_lines[index].offset;  // ADDED: off_t
        int   length = g_lines[index].length;

        printf("Строка %d: смещение = %lld, длина = %d\n",
               line_number, (long long)offset, length);
        printf("Содержимое: ");

        /* REMOVED: позиционирование и чтение через lseek/read
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("ошибка позиционирования");
            continue;
        }
        char buffer[MAX_LINE_LENGTH + 1];
        int bytes_read = read(fd, buffer, length);
        if (bytes_read > 0){
            buffer[bytes_read] = '\0';
            printf("'%s'\n", buffer);
        } else {
            printf("Ошибка чтения строки\n");
        }
        */

        /* ADDED: печать прямо из отображения — без копирования и без \0
           Заодно — защита на слишком длинные строки (чтобы не засорить терминал). */
        int to_print = length;
        if (to_print > MAX_LINE_LENGTH) {
            printf("[показано первые %d символов] '", MAX_LINE_LENGTH);
            printf("%.*s", MAX_LINE_LENGTH, g_base + offset);
            printf("'\n(полная длина строки = %d)\n", length);
        } else {
            printf("'%.*s'\n", to_print, g_base + offset);
        }

        // перезапускаем таймер на следующий ввод
        alarm(TIMEOUT);
    }

    /* ADDED: освобождаем отображение */
    if (g_base) munmap((void*)g_base, g_size);
    close(fd);
    return 0; 
}
