#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <sys/select.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    int server_fd, max_fd, activity, new_socket;
    int client_sockets[MAX_CLIENTS] = {0};
    struct sockaddr_un server_addr;
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
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
    
    while (1) {
        // Очищаем набор файловых дескрипторов
        FD_ZERO(&readfds);
        
        // Добавляем серверный сокет
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;
        
        // Добавляем клиентские сокеты
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > max_fd) {
                    max_fd = client_sockets[i];
                }
            }
        }
        
        // Ожидаем активности на сокетах
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        
        if ((activity < 0) && (errno != EINTR)) {
            perror("select");
        }
        
        // Проверяем новое подключение
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);
            if (new_socket == -1) {
                perror("accept");
                continue;
            }
            
            printf("Новый клиент подключен (fd: %d)\n", new_socket);
            
            // Добавляем новый сокет в массив
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        
        // Проверяем данные от клиентов
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                ssize_t bytes_read = read(sd, buffer, BUFFER_SIZE - 1);
                
                if (bytes_read == 0) {
                    // Соединение закрыто клиентом
                    printf("Клиент отключен (fd: %d)\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                } else if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    
                    // Преобразуем каждый символ в верхний регистр
                    for (int j = 0; j < bytes_read; j++) {
                        buffer[j] = toupper(buffer[j]);
                    }
                    
                    // Выводим результат с идентификатором клиента
                    printf("[Клиент %d]: %s", sd, buffer);
                    fflush(stdout);
                } else {
                    perror("read");
                    close(sd);
                    client_sockets[i] = 0;
                }
            }
        }
    }
    
    // Закрываем серверный сокет (эта строка никогда не выполнится в бесконечном цикле)
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}