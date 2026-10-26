/* 
 * TCP-клієнт для обміну повідомленнями із серверо
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE];

    char *server_ip = "127.0.0.1";
    if (argc > 1) {
        server_ip = argv[1]; // Дозволяє передати IP як аргумент
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Помилка: Не вдалося створити сокет \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    /* Конвертація IP-адреси з текстового у бінарний формат */
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\n Помилка: Некоректна адреса або адреса не підтримується \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Збій підключення: Не вдалося з'єднатися з сервером %s:%d \n", server_ip, PORT);
        return -1;
    }

    printf("=== ПІДКЛЮЧЕНО ДО СЕРВЕРА (%s:%d) ===\n", server_ip, PORT);
    printf("Введіть повідомлення для відправки (або 'EXIT' для виходу):\n> ");

    while (fgets(message, BUFFER_SIZE, stdin) != NULL) {
        // Видаляємо символ нового рядка з введення
        message[strcspn(message, "\r\n")] = 0;

        if (strlen(message) == 0) {
            printf("> ");
            continue;
        }
      
        send(sock, message, strlen(message), 0);

        // Отримання відповіді від сервера
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
        } else {
            printf("Сервер розірвав з'єднання.\n");
            break;
        }

        if (strcmp(message, "EXIT") == 0) {
            break;
        }

        printf("> ");
    }

    close(sock);
    return 0;
}
