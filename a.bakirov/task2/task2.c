#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main() {

    time_t now_t; // создаём объект для получения значения из функции time

    struct tm *sp; // создаем указатель на структуру 

    putenv("TZ=America/Los_Angeles"); // изменяем окружение процессора TZ на другой часовой пояс

    now_t = time(NULL); // столько-то секунд прошло с 1970 года в UTC

    time_t *a = &now_t; // инит указатель на адресс где лежит время с 1970

    printf("%s", ctime(a)); // высчитывает время которое сейчас наступило по UTC

    sp = localtime(a);  //    - Смотрит на TZ (America/Los_Angeles) и реагирует на смену в зимнее и летнее время UTC-8 UTC-7
                        //    - Вычитает 8 часов из UTC
                        //    - Заполняет структуру tm

    printf("%d/%d/%02d %d:%02d %s\n",
           sp->tm_mon + 1, sp->tm_mday, // + 1 чтобы было не от 0-11 а от 1-12
           sp->tm_year - 100, sp->tm_hour-1, // - 100 ну просто чтобы год без века был а то 2025 - 1900 = 125 но 125 - 100 = 25 как раз наш год
           sp->tm_min, tzname[0]);

    return 0;             
    // в 2038 году познаем pain перполнения time_t 
}
