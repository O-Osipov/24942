#include <stdio.h>
#include <unistd.h>// подключаем POSIX (write, pause)
// подключаем сигналы
#include <signal.h>

// глобальные счётчики, атомарные для сигналов
static volatile sig_atomic_t beeps = 0, quit = 0;

// обработчик SIGINT — подаём звуковой сигнал и увеличиваем счётчик
void on_int(int s){ 
    (void)s;                      // игнорируем номер сигнала
    write(STDOUT_FILENO, "\a", 1); // BEL — терминальный бип
    beeps++;                      // +1 к счётчику
}
// обработчик SIGQUIT — устанавливаем флаг выхода
void on_quit(int s){ 
    (void)s;      // игнорируем номер сигнала
    quit = 1;     // выставляем флаг завершения
}
int main(void){
    // при SIGINT (Ctrl-C) вызываем on_int
    signal(SIGINT,  on_int);
    // при SIGQUIT (Ctrl-\) вызываем on_quit
    signal(SIGQUIT, on_quit);    // ждём сигналы, пока флаг quit == 0
    while(!quit) 
        pause();          // процесс «спит» в ядре, пока не придёт сигнал
    printf("Количество бипов: %d\n", (int)beeps); // когда пришёл SIGQUIT — печатаем результат и завершаемся
    return 0; 
}
