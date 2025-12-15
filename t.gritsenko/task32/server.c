#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 100
#define MAX_CLIENTS 64
#define BACKLOG 5

static const char *socket_path = "./socket";

/* Таблица клиентов: храним только fd (для задачи этого достаточно). */
static int clients[MAX_CLIENTS];

/* Простая ASCII-версия upper (без toupper, чтобы не зависеть от locale
   и не вызывать не-async-signal-safe функции внутри обработчика). */
static char to_upper_ascii(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

/* Настраиваем fd как неблокирующий и асинхронный (SIGIO при событиях ввода). */
static void make_async_nonblock(int fd) {
    // F_GETFL / F_SETFL — получаем/ставим флаги файлового дескриптора
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        exit(1);
    }

    // O_NONBLOCK — read/accept не блокируют; O_ASYNC — генерировать SIGIO владельцу
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC) == -1) {
        perror("fcntl(F_SETFL)");
        exit(1);
    }

    // F_SETOWN — кому отправлять SIGIO (обычно текущему процессу)
    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl(F_SETOWN)");
        exit(1);
    }
}

/* Добавляем клиента в таблицу. Обновление таблицы делаем с блокировкой SIGIO,
   чтобы обработчик сигнала не читал массив одновременно. */
static void add_client(int fd) {
    sigset_t set, old;
    sigemptyset(&set);
    sigaddset(&set, SIGIO);
    sigprocmask(SIG_BLOCK, &set, &old);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] < 0) {
            clients[i] = fd;
            sigprocmask(SIG_SETMASK, &old, NULL);
            return;
        }
    }

    sigprocmask(SIG_SETMASK, &old, NULL);

    // Если клиентов слишком много — закрываем соединение
    close(fd);
}

/* Удаляем клиента (close + помечаем слот пустым). */
static void remove_client(int idx) {
    if (clients[idx] >= 0) {
        close(clients[idx]);
        clients[idx] = -1;
    }
}

/* Обработчик SIGIO: читаем доступные данные со всех клиентских сокетов.
   Важно: внутри handler используем только async-signal-safe вещи:
   read(), write(), close() и простую обработку буфера. */
static void sigio_handler(int sig) {
    (void)sig;

    char buf[BUFFER_SIZE];
    char out[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int fd = clients[i];
        if (fd < 0) continue;

        while (1) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n > 0) {
                for (ssize_t j = 0; j < n; j++) {
                    out[j] = to_upper_ascii(buf[j]);
                }
                // write в STDOUT — безопасно в сигнале
                (void)write(STDOUT_FILENO, out, (size_t)n);
                continue; // пробуем читать дальше (может быть ещё)
            }

            if (n == 0) {
                // EOF: клиент закрыл соединение
                remove_client(i);
                break;
            }

            // n < 0 — ошибка
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // данных сейчас нет — нормально для O_NONBLOCK
                break;
            }

            // любая другая ошибка — закрываем клиента
            remove_client(i);
            break;
        }
    }
}

int main(void) {
    // Инициализируем список клиентов как пустой
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    // socket(AF_UNIX, SOCK_STREAM, 0) — создаём Unix domain stream-сокет
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX; // домен Unix-сокетов (файловый путь)
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // unlink — удаляем старый файл сокета, если он остался после прошлого запуска
    unlink(socket_path);

    // bind — привязываем сокет к пути (создастся файл socket_path)
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    // listen — переводим сокет в режим ожидания входящих подключений
    if (listen(fd, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    // Настраиваем SIGIO обработчик
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigio_handler;
    sa.sa_flags = SA_RESTART;     // чтобы некоторые syscalls (например accept) перезапускались
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("sigaction(SIGIO)");
        return 1;
    }

    // Можно сделать серверный fd неблокирующим (не обязательно, но удобно)
    make_async_nonblock(fd);

    while (1) {
        // accept — принимаем новых клиентов
        int cl = accept(fd, NULL, NULL);
        if (cl == -1) {
            // Для неблокирующего accept нормально получать EAGAIN/EWOULDBLOCK
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Ждём сигналов (SIGIO) — “асинхронная” часть
                pause();
                continue;
            }
            perror("accept");
            continue;
        }

        // Каждый клиентский сокет делаем async+nonblock, чтобы он генерил SIGIO
        make_async_nonblock(cl);
        add_client(cl);
    }
}
