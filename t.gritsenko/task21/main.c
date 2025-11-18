#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int count = 0;

void handle_sigint(int signo) {
    printf("\a");
    fflush(NULL);
    count++;
}

void handle_sigquit(int signo) {
    printf("\nThe signal sounded %d times.\n", count);
    fflush(NULL);
    exit(0);
}

int main(void) {
    signal(SIGINT,  handle_sigint);
    signal(SIGQUIT, handle_sigquit);

    while (1)
        pause();

    return 0;
}
