#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>

static char *socket_path = "./socket"; // при желании переопредели через argv

int main(int argc, char **argv) {
    char ch = 'x';
    int count = 5000;      // сколько символов отправить
    int delay_us = 1000;   // задержка между байтами (в микросекундах)

    if (argc >= 2) ch = argv[1][0];          // 'x' или 'y'
    if (argc >= 3) count = atoi(argv[2]);    // количество
    if (argc >= 4) delay_us = atoi(argv[3]); // задержка

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); exit(1); }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(1);
    }

    for (int i = 0; i < count; i++) {
        ssize_t w = write(fd, &ch, 1);
        if (w != 1) {
            if (w == -1) perror("write");
            else fprintf(stderr, "partial write\n");
            break;
        }
        if (delay_us > 0) usleep(delay_us);
    }

    close(fd);
    return 0;
}
