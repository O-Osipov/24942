// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.
// Необходимо обеспечить возможность подключения нескольких клиентов и параллельное (без задержек) получение текста от них.  При этом, преобразованный текст разных клиентов в выдаче сервера может смешиваться.
// Реализуйте задачу 31, используя асинхронный ввод-вывод вместо select(3C)/poll(2).

// ./server & sleep 1 && (echo "Client1" | ./client & echo "Client2" | ./client & echo "Client3" | ./client & wait) && kill %1

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <aio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

static const char *socket_path = "./socket30";

#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

typedef struct {
    int fd;
    struct aiocb aio_read;
    char buffer[BUFFER_SIZE];
    int active;
} client_t;

static client_t clients[MAX_CLIENTS];
static int active_clients = 0;

static int start_async_read(client_t *client) {
    memset(&client->aio_read, 0, sizeof(client->aio_read));
    client->aio_read.aio_fildes = client->fd;
    client->aio_read.aio_buf = client->buffer;
    client->aio_read.aio_nbytes = BUFFER_SIZE;
    client->aio_read.aio_offset = 0;

    if (aio_read(&client->aio_read) == -1) {
        perror("aio_read");
        return -1;
    }
    return 0;
}

static int add_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            continue;
        }

        client_t *client = &clients[i];
        client->fd = fd;
        client->active = 1;

        if (start_async_read(client) == -1) {
            client->active = 0;
            close(fd);
            return -1;
        }

        active_clients++;
        return 0;
    }

    close(fd);
    return -1;
}

static void remove_client(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return;
    if (!clients[index].active) return;
    
    aio_cancel(clients[index].fd, &clients[index].aio_read);
    close(clients[index].fd);
    clients[index].active = 0;
    active_clients--;
}

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
    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    // Make listening socket non-blocking
    int flags = fcntl(listen_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(listen_fd);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    for (;;) {
        // Accept new connections (non-blocking)
        for (;;) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; // No more connections
                }
                if (errno == EINTR) continue;
                perror("accept");
                break;
            }
            add_client(client_fd);
        }

        // Check for completed async reads
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].active) continue;
            
            int err = aio_error(&clients[i].aio_read);
            if (err == EINPROGRESS) {
                continue; // Still in progress
            }
            
            if (err != 0) {
                // Error occurred
                if (err != ECANCELED) {
                    perror("aio_error");
                }
                remove_client(i);
                continue;
            }
            
            // Read completed
            ssize_t n = aio_return(&clients[i].aio_read);
            if (n == 0) {
                // Client disconnected
                remove_client(i);
                continue;
            }
            
            if (n < 0) {
                perror("aio_return");
                remove_client(i);
                continue;
            }
            
            // Process and output data
            for (ssize_t j = 0; j < n; ++j) {
                unsigned char ch = (unsigned char)clients[i].buffer[j];
                if (isalpha(ch)) {
                    clients[i].buffer[j] = (char)toupper(ch);
                } else {
                    clients[i].buffer[j] = (char)ch;
                }
            }

            if (robust_write(STDOUT_FILENO, clients[i].buffer, (size_t)n) < 0) {
                perror("write");
            }
            
            // Start next async read
            if (start_async_read(&clients[i]) == -1) {
                remove_client(i);
                continue;
            }
        }

        // Wait for at least one async operation to complete
        if (active_clients > 0) {
            struct aiocb *list[MAX_CLIENTS];
            int count = 0;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i].active) {
                    list[count++] = &clients[i].aio_read;
                }
            }
            if (count > 0) {
                struct timespec timeout;
                timeout.tv_sec = 0;
                timeout.tv_nsec = 100000000; // 100ms
                if (aio_suspend((const struct aiocb *const *)list, count, &timeout) == -1) {
                    if (errno != EINTR && errno != EAGAIN) {
                        perror("aio_suspend");
                    }
                }
            }
        } else {
            // No clients, sleep briefly before checking for new connections
            usleep(100000); // 100ms
        }
    }

    // Cleanup: close all remaining file descriptors
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            aio_cancel(clients[i].fd, &clients[i].aio_read);
            close(clients[i].fd);
        }
    }
    close(listen_fd);

    return 0;
}