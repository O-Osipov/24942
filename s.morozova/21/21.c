#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t count = 0;

void handle_sigint(int sig) { count++; write(1, "\a", 1); }
void handle_sigquit(int sig) { printf("\n%d\n", count); exit(0); }

int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, handle_sigquit);
    
    while (1) pause();
    return 0;
}