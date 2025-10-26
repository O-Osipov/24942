// Напишите программу, которая входит в бесконечный цикл и издает звуковой сигнал на вашем терминале каждый раз, когда вы вводите символ, на который у вас настроена посылка сигнала SIGINT (по умолчанию CTRL-C). При получении SIGQUIT, она должна вывести сообщение, говорящее, сколько раз прозвучал сигнал, и завершиться.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int count = 0;

void handleSIGINT() {
    printf("\a");
    fflush(stdout);
    count++;
}

void handleSIGQUIT() {
    printf("\nThe signal sounded %d times.\n", count);
    exit(0);
}

int main() {
    signal(SIGINT, handleSIGINT);
    signal(SIGQUIT, handleSIGQUIT);
    
    printf("Program started. Press CTRL-C to make sound, CTRL-\\ to quit.\n");
    printf("Sound count: %d\n", count);
    
    while (1) {
        sleep(1);
    }
    
    return 0;
}