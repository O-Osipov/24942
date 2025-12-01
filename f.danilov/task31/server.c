#define _GNU_SOURCE
// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define SOCKET_PATH "task31_socket"
#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int messages_received;
    char buf[BUFFER_SIZE];  // буфер для накопления данных
    size_t buf_len;         // сколько байт в буфере
} client_state_t;

int main() {
    unlink(SOCKET_PATH);

    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("Server listening on %s\n", SOCKET_PATH);

    struct pollfd fds[MAX_CLIENTS + 1];
    client_state_t clients[MAX_CLIENTS] = {0};
    int completed_clients = 0;

    int nfds = 1;
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].fd = -1;
        clients[i].messages_received = 0;
        clients[i].buf_len = 0;
        fds[i + 1].fd = -1;
        fds[i + 1].events = POLLIN;
    }

    while (1) {
        int ready = poll(fds, nfds, -1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // Новое подключение
        if (fds[0].revents & POLLIN) {
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd == -1) {
                perror("accept");
            } else {
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (clients[i].fd == -1) {
                        slot = i;
                        break;
                    }
                }
                if (slot == -1) {
                    close(client_fd);
                } else {
                    clients[slot].fd = client_fd;
                    clients[slot].messages_received = 0;
                    clients[slot].buf_len = 0;
                    fds[slot + 1].fd = client_fd;
                    if (slot + 2 > nfds) nfds = slot + 2;
                }
            }
        }

        // Обработка данных
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd == -1) continue;

            int idx = i + 1;
            if (fds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                char tmp_buf[BUFFER_SIZE];
                ssize_t n = read(clients[i].fd, tmp_buf, sizeof(tmp_buf));
                if (n <= 0) {
                    // Клиент отключился — обрабатываем остатки в буфере
                    if (clients[i].buf_len > 0) {
                        // Считаем остаток как последнее сообщение (даже без \n)
                        for (size_t j = 0; j < clients[i].buf_len; ++j) {
                            clients[i].buf[j] = toupper((unsigned char)clients[i].buf[j]);
                        }
                        write(STDOUT_FILENO, clients[i].buf, clients[i].buf_len);
                        clients[i].messages_received++;
                    }
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    fds[idx].fd = -1;
                    while (nfds > 1 && fds[nfds - 1].fd == -1) nfds--;

                    if (clients[i].messages_received >= 2) {
                        completed_clients++;
                        if (completed_clients >= 2) {
                                struct timespec end_time;
                                clock_gettime(CLOCK_MONOTONIC, &end_time);
                                
                                double duration = (end_time.tv_sec - start_time.tv_sec) +
                                                (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

                                for (int i = 0; i < MAX_CLIENTS; ++i) {
                                    if (clients[i].fd != -1) {
                                        close(clients[i].fd);
                                    }
                                }
                                close(server_socket);
                                unlink(SOCKET_PATH);

                                printf("\nServer stopped. Total processing time: %.3f seconds\n", duration);
                                return 0;
                        }
                    }
                } else {
                    // Добавляем в буфер
                    if (clients[i].buf_len + n > BUFFER_SIZE - 1) {
                        // Переполнение — отбрасываем
                        clients[i].buf_len = 0;
                    }
                    memcpy(clients[i].buf + clients[i].buf_len, tmp_buf, n);
                    clients[i].buf_len += n;

                    // Обрабатываем все полные строки в буфере
                    while (1) {
                        char *newline = memchr(clients[i].buf, '\n', clients[i].buf_len);
                        if (!newline) break;

                        size_t line_len = newline - clients[i].buf + 1;
                        for (size_t j = 0; j < line_len; ++j) {
                            clients[i].buf[j] = toupper((unsigned char)clients[i].buf[j]);
                        }
                        write(STDOUT_FILENO, clients[i].buf, line_len);

                        clients[i].messages_received++;

                        // Удаляем строку из буфера
                        memmove(clients[i].buf, newline + 1, clients[i].buf_len - line_len);
                        clients[i].buf_len -= line_len;

                        if (clients[i].messages_received >= 2) {
                            // Закрываем клиента
                            close(clients[i].fd);
                            clients[i].fd = -1;
                            fds[idx].fd = -1;
                            while (nfds > 1 && fds[nfds - 1].fd == -1) nfds--;

                            completed_clients++;
                            if (completed_clients >= 2) {
                                struct timespec end_time;
                                clock_gettime(CLOCK_MONOTONIC, &end_time);
                                
                                double duration = (end_time.tv_sec - start_time.tv_sec) +
                                                (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

                                for (int i = 0; i < MAX_CLIENTS; ++i) {
                                    if (clients[i].fd != -1) {
                                        close(clients[i].fd);
                                    }
                                }
                                close(server_socket);
                                unlink(SOCKET_PATH);

                                printf("\nServer stopped. Total processing time: %.3f seconds\n", duration);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
}