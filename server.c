/* 
 * server.c — багатопотоковий TCP-сервер для обміну повідомленнями
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
} client_info_t;

void *handle_client(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    int sock = client->socket;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("[КЕРУВАННЯ] Підключено нового клієнта з IP: %s:%d\n", client_ip, ntohs(client->address.sin_port));

    free(client); // Звільняємо пам'ять, виділену в головному потоці

    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        buffer[strcspn(buffer, "\r\n")] = 0;

        printf("[%s:%d] Повідомлення: %s\n", client_ip, ntohs(client->address.sin_port), buffer);

        if (strcmp(buffer, "EXIT") == 0) {
            printf("[КЕРУВАННЯ] Клієнт %s ініціював закриття з'єднання.\n", client_ip);
            char *response = "SERVER: З'єднання закрито за запитом.\n";
            send(sock, response, strlen(response), 0);
            break;
        }

        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "SERVER_ACK: Отримано [%s]\n", buffer);
        send(sock, response, strlen(response), 0);
    }

    if (bytes_read == 0) {
        printf("[КЕРУВАННЯ] Клієнт %s відключився.\n", client_ip);
    } else if (bytes_read < 0) {
        perror("Помилка читання з сокета (recv)");
    }

    close(sock);
    pthread_exit(NULL);
}

int main(void) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Помилка створення сокета");
        exit(EXIT_FAILURE);
    }

    /* Налаштування повторного використання порту */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Помилка setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Помилка прив'язки (bind)");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Помилка listen");
        exit(EXIT_FAILURE);
    }

    printf("=== TCP СЕРВЕР ЗАПУЩЕНО НА ПОРТУ %d ===\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Помилка accept");
            continue;
        }

        client_info_t *client = malloc(sizeof(client_info_t));
        if (!client) {
            perror("Помилка виділення пам'яті для клієнта");
            close(new_socket);
            continue;
        }
        client->socket = new_socket;
        client->address = address;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client) != 0) {
            perror("Помилка створення потоку");
            free(client);
            close(new_socket);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
