// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.
// Необходимо обеспечить возможность подключения нескольких клиентов и параллельное (без задержек) получение текста от них.  При этом, преобразованный текст разных клиентов в выдаче сервера может смешиваться.
// Реализуйте задачу 31, используя асинхронный ввод-вывод вместо select(3C)/poll(2).

// ./server & server_pid=$! && sleep 1 && (echo "Client1" | ./client & echo "Client2" | ./client & echo "Client3" | ./client & wait) && kill "$server_pid" && wait "$server_pid" 2>/dev/null

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <aio.h>

static const char *socket_path = "./socket30";

#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

typedef struct {
    int fd;
    struct aiocb aio;
    char buffer[BUFFER_SIZE];
    int active;
    int pending;
} client_info_t;

static ssize_t robust_write(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }
    return (ssize_t)count;
}

int main(void) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    // Make listening socket non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    fd_set read_fds, master_fds;
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);

    client_info_t clients[MAX_CLIENTS];
    int had_clients = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
        clients[i].pending = 0;
        memset(&clients[i].aio, 0, sizeof(struct aiocb));
    }

    for (;;) {
        read_fds = master_fds;

        // Check if we had clients and all disconnected
        int active_before = 0;
        int pending_before = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                active_before++;
            }
            if (clients[i].pending) {
                pending_before++;
            }
        }

        struct timeval timeout;
        struct timeval *timeout_ptr = NULL;
        
        // If we had clients and all disconnected with no pending operations, use timeout
        if (had_clients && active_before == 0 && pending_before == 0) {
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000; // 200ms timeout
            timeout_ptr = &timeout;
        } else {
            timeout_ptr = NULL; // Block indefinitely
        }

        int nready = select(server_fd + 1, &read_fds, NULL, NULL, timeout_ptr);
        if (nready == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        
        // If timeout occurred and we had clients but all disconnected, exit
        if (nready == 0 && had_clients && active_before == 0 && pending_before == 0) {
            break;
        }

        // Accept new connections (non-blocking)
        if (FD_ISSET(server_fd, &read_fds)) {
            for (;;) {
                int new_client = accept(server_fd, NULL, NULL);
                if (new_client == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break; // No more connections
                    }
                    if (errno == EINTR) continue;
                    perror("accept");
                    break;
                }

                // Find free slot for new client
                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = new_client;
                        clients[i].active = 1;
                        clients[i].pending = 0;

                        memset(&clients[i].aio, 0, sizeof(struct aiocb));
                        clients[i].aio.aio_fildes = new_client;
                        clients[i].aio.aio_buf = clients[i].buffer;
                        clients[i].aio.aio_nbytes = BUFFER_SIZE;
                        clients[i].aio.aio_offset = 0;

                        if (aio_read(&clients[i].aio) == -1) {
                            perror("aio_read");
                            close(new_client);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                        } else {
                            clients[i].pending = 1;
                            had_clients = 1;
                        }
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    close(new_client);
                }
            }
        }

        // Check for completed async reads
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && clients[i].pending) {
                int error = aio_error(&clients[i].aio);

                if (error == EINPROGRESS) {
                    continue; // Still in progress
                }

                clients[i].pending = 0;
                ssize_t nbytes = aio_return(&clients[i].aio);

                if (nbytes <= 0) {
                    if (nbytes == -1 && error != ECANCELED) {
                        perror("aio_return");
                    }
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                } else {
                    // Measure processing time and output data
                    struct timespec start_time, end_time;
                    size_t write_pos = 0;
                    clock_gettime(CLOCK_MONOTONIC, &start_time);
                    for (ssize_t j = 0; j < nbytes; ++j) {
                        unsigned char ch = (unsigned char)clients[i].buffer[j];
                        // Filter out control characters that might trigger commands
                        if (isalnum(ch) || isprint(ch)) {
                            clients[i].buffer[write_pos] = (char)(isalpha(ch) ? toupper(ch) : ch);
                            write_pos++;
                        }
                    }
                    nbytes = (ssize_t)write_pos;

                    if (nbytes > 0) {
                        clock_gettime(CLOCK_MONOTONIC, &end_time);
                        long processing_us =
                            (end_time.tv_sec - start_time.tv_sec) * 1000000L +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1000L;
                        char processing_info[64];
                        snprintf(processing_info, sizeof(processing_info),
                                 "[Processing time: %ld us] ", processing_us);

                        // Get current time with millisecond precision and format timestamp
                        struct timeval tv;
                        struct tm *timeinfo;
                        char timestamp[64];

                        gettimeofday(&tv, NULL);
                        timeinfo = localtime(&tv.tv_sec);
                        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S", timeinfo);
                        snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp), ".%03ld] ", (long)(tv.tv_usec / 1000));

                        // Write timestamp
                        if (robust_write(STDOUT_FILENO, timestamp, strlen(timestamp)) < 0) {
                            perror("write");
                        }
                        // Write processing info
                        if (robust_write(STDOUT_FILENO, processing_info, strlen(processing_info)) < 0) {
                            perror("write");
                        }
                        // Write processed data
                        if (robust_write(STDOUT_FILENO, clients[i].buffer, (size_t)nbytes) < 0) {
                            perror("write");
                        }
                        // Add newline after output for next input
                        if (robust_write(STDOUT_FILENO, "\n", 1) < 0) {
                            perror("write");
                        }
                    }

                    // Start next async read
                    memset(&clients[i].aio, 0, sizeof(struct aiocb));
                    clients[i].aio.aio_fildes = clients[i].fd;
                    clients[i].aio.aio_buf = clients[i].buffer;
                    clients[i].aio.aio_nbytes = BUFFER_SIZE;
                    clients[i].aio.aio_offset = 0;

                    if (aio_read(&clients[i].aio) == -1) {
                        if (errno != EINPROGRESS) {
                            perror("aio_read");
                            close(clients[i].fd);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                        } else {
                            clients[i].pending = 1;
                        }
                    } else {
                        clients[i].pending = 1;
                    }
                }
            }
        }
    }

    // Cleanup: cancel pending operations and close all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            aio_cancel(clients[i].fd, NULL);
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }

    close(server_fd);
    unlink(socket_path);
    return 0;
}
