#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

extern char *tzname[];

int main()
{
    time_t now;
    time(&now);

    setlocale(LC_TIME, "");

    setenv("TZ", "PST8", 1);
    tzset();

    struct tm *sp = localtime(&now);

    printf("Текущее время: %s\n", asctime(sp));

    return 0;
}