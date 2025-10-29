#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 40
#define ERASE 0x7F
#define KILL 0x15
#define CTRL_W 0x17
#define CTRL_D 0x04
#define BELL 0x07

char buf[256];
int len = 0;
int col = 0;

void init_term() {
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &t);
}

void reterm() {
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(0, TCSANOW, &t);
}

void redraw() {
    int i;
    for (i = 0; i < len + 2; i++) putchar('\b');
    for (i = 0; i < len + 2; i++) putchar(' ');
    for (i = 0; i < len + 2; i++) putchar('\b');
    
    col = 0;
    for (i = 0; i < len; i++) {
        putchar(buf[i]);
        col++;
        if (col >= MAX_LEN && i < len - 1) {
            putchar('\n');
            col = 0;
        }
    }
    fflush(stdout);
}

void del_word() {
    if (len == 0) { putchar(BELL); return; }
    int end = len - 1;
    while (end >= 0 && isspace(buf[end])) end--;
    while (end >= 0 && !isspace(buf[end])) end--;
    len = end + 1;
}

int main() {
    init_term();
    
    char c;
    while (1) {
        c = getchar();
        
        if (c == CTRL_D && len == 0) break;
        
        if (c == ERASE) {
            if (len > 0) len--;
            else putchar(BELL);
        }
        else if (c == KILL) {
            len = 0;
        }
        else if (c == CTRL_W) {
            del_word();
        }
        else if (c >= 32 && c <= 126) {
            if (len < 255) buf[len++] = c;
            else putchar(BELL);
        }
        else {
            putchar(BELL);
            continue;
        }
        
        redraw();
    }
    
    reterm();
    printf("\n");
    return 0;
}