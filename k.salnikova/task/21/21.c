#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int beep_count = 0;

void siginth(int sig) {
    write(1, "\a", 1);
    beep_count++;
}

void sigquith(int sig) {
    printf("\nBeep count: %d\n", beep_count);
    exit(0);
}

int main() {
    signal(SIGINT, siginth);
    signal(SIGQUIT, sigquith);
    
    while(1) {
        pause();
    }
}