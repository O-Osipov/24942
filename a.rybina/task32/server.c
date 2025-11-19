// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.
// Необходимо обеспечить возможность подключения нескольких клиентов и параллельное (без задержек) получение текста от них.  При этом, преобразованный текст разных клиентов в выдаче сервера может смешиваться.
// Реализуйте задачу 31, используя асинхронный ввод-вывод вместо select(3C)/poll(2).

// ./server & sleep 1 && (echo "Client1" | ./client & echo "Client2" | ./client & echo "Client3" | ./client & wait) && kill %1

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <liburing.h>

static const char *socket_path = "./socket30";

#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192
#define QUEUE_DEPTH 256

typedef enum {
    OP_ACCEPT,
    OP_CLIENT_READ
} op_type_t;

typedef struct {
    op_type_t type;
    int client_index;
} io_data_t;

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int active;
    io_data_t read_data;
} client_t;

static client_t clients[MAX_CLIENTS];
static struct io_uring ring;
static io_data_t accept_data;

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

static void remove_client(int index) {
    if (index < 0 || index >= MAX_CLIENTS) {
        return;
    }
    if (!clients[index].active) {
        return;
    }

    close(clients[index].fd);
    clients[index].fd = -1;
    clients[index].active = 0;
}

static struct io_uring_sqe *get_sqe_blocking(void) {
    struct io_uring_sqe *sqe;
    while ((sqe = io_uring_get_sqe(&ring)) == NULL) {
        int ret = io_uring_submit(&ring);
        if (ret < 0) {
            errno = -ret;
            return NULL;
        }
    }
    return sqe;
}

static int submit_accept(int listen_fd) {
    struct io_uring_sqe *sqe = get_sqe_blocking();
    if (!sqe) {
        return -1;
    }

    accept_data.type = OP_ACCEPT;
    accept_data.client_index = -1;
    io_uring_prep_accept(sqe, listen_fd, NULL, NULL, 0);
    io_uring_sqe_set_data(sqe, &accept_data);
    return 0;
}

static int start_client_read(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return -1;
    if (!clients[index].active) return -1;

    struct io_uring_sqe *sqe = get_sqe_blocking();
    if (!sqe) {
        return -1;
    }

    clients[index].read_data.type = OP_CLIENT_READ;
    clients[index].read_data.client_index = index;

    io_uring_prep_read(sqe, clients[index].fd, clients[index].buffer, BUFFER_SIZE, 0);
    io_uring_sqe_set_data(sqe, &clients[index].read_data);
    return 0;
}

static int add_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            continue;
        }
        clients[i].fd = fd;
        clients[i].active = 1;
        if (start_client_read(i) == -1) {
            clients[i].active = 0;
            close(fd);
            return -1;
        }
        return 0;
    }
    close(fd);
    return -1;
}

int main(void) {
    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
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

    if (listen(listen_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        close(listen_fd);
        return 1;
    }

    if (submit_accept(listen_fd) == -1) {
        perror("submit_accept");
        io_uring_queue_exit(&ring);
        close(listen_fd);
        return 1;
    }

    while (1) {
        struct io_uring_cqe *cqe;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            if (ret == -EINTR) {
                continue;
            }
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
            break;
        }

        io_data_t *data = io_uring_cqe_get_data(cqe);
        int result = cqe->res;

        if (!data) {
            io_uring_cqe_seen(&ring, cqe);
            continue;
        }

        if (data->type == OP_ACCEPT) {
            if (result >= 0) {
                int client_fd = result;
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags != -1) {
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                }
                if (add_client(client_fd) == -1) {
                    fprintf(stderr, "Too many clients, closing connection\n");
                }
            } else {
                if (result != -EAGAIN && result != -ECANCELED) {
                    fprintf(stderr, "accept error: %s\n", strerror(-result));
                }
            }

            if (submit_accept(listen_fd) == -1) {
                perror("submit_accept");
                io_uring_cqe_seen(&ring, cqe);
                break;
            }
        } else if (data->type == OP_CLIENT_READ) {
            int idx = data->client_index;
            if (result <= 0) {
                if (result < 0 && result != -ECANCELED && result != -EAGAIN) {
                    fprintf(stderr, "read error: %s\n", strerror(-result));
                }
                remove_client(idx);
            } else {
                ssize_t n = result;
                for (ssize_t j = 0; j < n; ++j) {
                    unsigned char ch = (unsigned char)clients[idx].buffer[j];
                    if (isalpha(ch)) {
                        clients[idx].buffer[j] = (char)toupper(ch);
                    } else {
                        clients[idx].buffer[j] = (char)ch;
                    }
                }
                if (robust_write(STDOUT_FILENO, clients[idx].buffer, (size_t)n) == -1) {
                    perror("write");
                }
                if (start_client_read(idx) == -1) {
                    remove_client(idx);
                }
            }
        }

        io_uring_cqe_seen(&ring, cqe);
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }

    io_uring_queue_exit(&ring);
    close(listen_fd);
    unlink(socket_path);
    return 0;
}