// Напишите программу, которая входит в бесконечный цикл и издает звуковой сигнал на вашем терминале каждый раз, когда вы вводите символ, на который у вас настроена посылка сигнала SIGINT (по умолчанию CTRL-C). При получении SIGQUIT, она должна вывести сообщение, говорящее, сколько раз прозвучал сигнал, и завершиться.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// For Windows compatibility
#ifndef SIGQUIT
#define SIGQUIT SIGTERM
#endif

int count = 0;
time_t last_sigint_time = 0;
const int DOUBLE_PRESS_WINDOW = 1; // seconds

void handleSIGINT(int sig) {
    (void)sig;  // Suppress unused parameter warning
    time_t current_time = time(NULL);

    // Make sound and increment count first (for both single and double press)
    printf("\a");  // Make sound
    fflush(stdout);
    count++;
    printf(" [Sound #%d] ", count);  // Debug output
    fflush(stdout);

    // Check if this is a double press (within 1 second of last press)
    if (last_sigint_time != 0 && (current_time - last_sigint_time) < DOUBLE_PRESS_WINDOW) {
        printf("\nThe signal sounded %d times.\n", count);
        exit(0);
    }

    last_sigint_time = current_time;

    // Re-register the signal handler (some systems reset it after first call)
    signal(SIGINT, handleSIGINT);
}

void handleSIGQUIT(int sig) {
    (void)sig;  // Suppress unused parameter warning
    printf("\nThe signal sounded %d times.\n", count);
    exit(0);
}

int main() {
    signal(SIGINT, handleSIGINT);
    signal(SIGQUIT, handleSIGQUIT);
    
    printf("Program started. Press CTRL-C to make sound, CTRL-\\ to quit.\n");
    printf("On Windows: Press CTRL-C twice quickly to quit.\n");
    printf("Sound count: %d\n", count);
    
    while (1) {
        sleep(1);
    }
    
    return 0;
}