#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int beep_count = 0;

void siginth(int sig) {
    printf("\nBeep count: %d\n", beep_count);
    exit(0);
}

int main() {
    signal(SIGINT, siginth);
    
    printf("Program started. Type text and press Enter to beep. Press CTRL-D to exit.\n");
    
    while(1) {
        int c = getchar();
        
        if (c == EOF) {
            printf("\nBeep count: %d\n", beep_count);
            exit(0);
        }
        
        if (c == '\n') {
            write(1, "\a", 1); 
            beep_count++;
            printf("Beep! (line completed)\n");
        }
    }
    
    return 0;
}