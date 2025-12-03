#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>

#define SOCKET "/tmp/mysocket"

// Чистим все завершившиеся процессы 
void sigchld_handler(int signum) 
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() 
{
    // После завершения процесса выполняем функцию
    signal(SIGCHLD, sigchld_handler);

    // Дескриптор для сервера
    int server_fd;

    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) 
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Структура для записи адреса сервера
    struct sockaddr_un server_addr = {0};
    // Говорим что используется семейство UNIX сокетов
    server_addr.sun_family = AF_UNIX;
    // Устанавливаем путь к файлу сокета 
    strncpy(server_addr.sun_path, SOCKET, sizeof(server_addr.sun_path) - 1);

    // Удаляем старый сокет, если он существует
    unlink(SOCKET);
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, 5) == -1) 
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен и слушает\n");

    while (1) 
    {
        // Дескриптор клиента
        int client_fd;
        // Структура адреса клиента
        struct sockaddr_un client_addr = {0};
        socklen_t client_len = sizeof(client_addr);

        // Ждем соединения с клиентом
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) 
        {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        printf("Клиент подключен\n");

        // Создаём новый процесс для каждого клиента
        pid_t pid = fork();
        // Дочерний процесс
        if (pid == 0) 
        {
            // Закрываем серверный сокет в дочернем
            close(server_fd);

            char buffer[500];
            long bytes_read;

            // Читаем данные от клиента и преобразуем в верхний регистр
            while ((bytes_read = read(client_fd, buffer, 500)) > 0) 
            {
                buffer[bytes_read] = '\0'; // Добавляем нулевой терминатор
                
                // Преобразуем в верхний регистр
                for (int i = 0; i < bytes_read; i++) 
                {
                    buffer[i] = toupper(buffer[i]);
                }
                
                // Выводим результат
                printf("%s", buffer);
                fflush(stdout);
            }
            if (bytes_read == 0) 
            {
                printf("Клиент отключился\n");
                close(client_fd);
                exit(0); // завершаем дочерний процесс
            }

            else if (bytes_read == -1) 
            {
                perror("read");
            }
        } 
        // Родительский процесс
        else if (pid > 0) 
        {
            close(client_fd); // Закрываем дескриптор клиента в родителе
        } 
        else 
        {
            perror("fork");
            close(client_fd);
        }
    }

    close(server_fd);
    unlink(SOCKET);
    return 0;
}
