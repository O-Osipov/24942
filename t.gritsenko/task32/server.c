#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <aio.h>
#include <signal.h>

#define BUFFER_SIZE 100
#define BACKLOG 5

char *socket_path = "./socket";

/* Создаёт AIO-запрос для чтения из сокета */
struct aiocb *create_request(int fd) {
    struct aiocb *req = calloc(1, sizeof(struct aiocb));

    req->aio_fildes = fd;                 // файловый дескриптор сокета
    req->aio_buf = malloc(BUFFER_SIZE);   // буфер для чтения
    req->aio_nbytes = BUFFER_SIZE;        // сколько читать

    // Настройка сигнального уведомления
    req->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    req->aio_sigevent.sigev_signo = SIGUSR1;
    req->aio_sigevent.sigev_value.sival_ptr = req;

    return req;
}

/* Обработчик сигнала завершения aio_read */
void aio_handler(int sig, siginfo_t *info, void *ctx) {
    struct aiocb *req = info->si_value.sival_ptr;

    if (aio_error(req) != 0)
        return;

    ssize_t n = aio_return(req);
    char *buf = (char *)req->aio_buf;

    if (n == 0) {
        // EOF — клиент закрыл соединение
        close(req->aio_fildes);
        free(buf);
        free(req);
        return;
    }

    // Преобразуем в верхний регистр и печатаем
    for (int i = 0; i < n; i++) {
        putchar(toupper(buf[i]));
    }
    fflush(stdout);

    // Снова ставим асинхронное чтение
    aio_read(req);
}

int main() {
    int fd, cl;

    // Создание Unix domain socket
    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, BACKLOG);

    // Установка обработчика сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = aio_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        // Принимаем клиентов обычным accept()
        cl = accept(fd, NULL, NULL);

        struct aiocb *req = create_request(cl);
        aio_read(req);   // запускаем асинхронное чтение
    }
}
