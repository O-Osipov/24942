#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "task31_socket"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <1|2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int id = atoi(argv[1]);
    if (id != 1 && id != 2) {
        fprintf(stderr, "ID must be 1 or 2\n");
        exit(EXIT_FAILURE);
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (id == 1) {
        char m1[] = "Client1 first message: Hello!\n";
        char m2[] = "Client1 second message: How are you?\n";
        write(sock, m1, strlen(m1));
        write(sock, m2, strlen(m2));
    } else {
        char m1[] = "Client2 first message: Hi there!\n";
        char m2[] = "Client2 second message: All good!\n";
        write(sock, m1, strlen(m1));
        write(sock, m2, strlen(m2));
    }

    close(sock);
    return 0;
}