#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/case_converter_aio_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int fd;
    struct aiocb read_cb;
    char read_buffer[BUFFER_SIZE];
    int active;
} client_t;

client_t clients[MAX_CLIENTS];
int server_fd;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void process_client_data(client_t *client) {
    // Проверяем завершение операции чтения
    int read_status = aio_error(&client->read_cb);
    if (read_status == 0) {
        ssize_t bytes_read = aio_return(&client->read_cb);
        
        if (bytes_read > 0) {
            // Преобразуем в верхний регистр и выводим
            printf("[Клиент %d]: ", client->fd);
            for (int i = 0; i < bytes_read; i++) {
                char c = toupper(client->read_buffer[i]);
                putchar(c);
            }
            fflush(stdout);
            
            // Инициируем новое чтение
            memset(&client->read_cb, 0, sizeof(struct aiocb));
            client->read_cb.aio_fildes = client->fd;
            client->read_cb.aio_buf = client->read_buffer;
            client->read_cb.aio_nbytes = BUFFER_SIZE - 1;
            
            if (aio_read(&client->read_cb) == -1) {
                perror("aio_read");
                client->active = 0;
                close(client->fd);
            }
        } else if (bytes_read == 0) {
            // Соединение закрыто
            printf("Клиент отключен (fd: %d)\n", client->fd);
            client->active = 0;
            close(client->fd);
        } else {
            perror("read error");
            client->active = 0;
            close(client->fd);
        }
    } else if (read_status != EINPROGRESS) {
        perror("aio_error");
        client->active = 0;
        close(client->fd);
    }
}

void *aio_monitor_thread(void *arg) {
    printf("Монитор запущен\n");
    
    while (1) {
        pthread_mutex_lock(&clients_mutex);
        
        int active_clients = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                active_clients++;
                process_client_data(&clients[i]);
            }
        }
        
        pthread_mutex_unlock(&clients_mutex);
        usleep(10000); // 10ms для уменьшения нагрузки на CPU
    }
    return NULL;
}

void accept_new_clients() {
    int new_socket = accept(server_fd, NULL, NULL);
    if (new_socket == -1) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("accept");
        }
        return;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    // Ищем свободный слот для нового клиента
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = new_socket;
            clients[i].active = 1;
            
            // Настраиваем AIO для чтения
            memset(&clients[i].read_cb, 0, sizeof(struct aiocb));
            clients[i].read_cb.aio_fildes = new_socket;
            clients[i].read_cb.aio_buf = clients[i].read_buffer;
            clients[i].read_cb.aio_nbytes = BUFFER_SIZE - 1;
            
            if (aio_read(&clients[i].read_cb) == -1) {
                perror("aio_read for new client");
                clients[i].active = 0;
                close(new_socket);
            } else {
                printf("Новый клиент подключен (fd: %d)\n", new_socket);
                added = 1;
            }
            break;
        }
    }
    
    if (!added) {
        printf("Достигнут лимит клиентов, отказываем в подключении\n");
        close(new_socket);
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void cleanup() {
    printf("\nОчистка ресурсов...\n");
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    if (server_fd >= 0) {
        close(server_fd);
    }
    unlink(SOCKET_PATH);
}

int main() {
    struct sockaddr_un server_addr;
    pthread_t monitor_thread;
    
    // Регистрируем обработчик очистки
    atexit(cleanup);
    
    // Инициализируем структуры клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
    }
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем неблокирующий режим
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl O_NONBLOCK");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Удаляем старый сокет если он существует
    unlink(SOCKET_PATH);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем прослушивание
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен и ожидает подключения...\n");
    printf("Socket path: %s\n", SOCKET_PATH);
    printf("Максимальное количество клиентов: %d\n", MAX_CLIENTS);
    
    // Запускаем поток для мониторинга AIO операций
    if (pthread_create(&monitor_thread, NULL, aio_monitor_thread, NULL) != 0) {
        perror("pthread_create");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    pthread_detach(monitor_thread);
    
    // Основной цикл принятия новых подключений
    printf("Основной цикл запущен\n");
    while (1) {
        accept_new_clients();
        usleep(10000); // 10ms пауза
    }
    
    return 0;
}