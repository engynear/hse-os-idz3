#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 256

enum Component {
    Tobacco = 1,
    Paper = 2,
    Matches = 3
};

const char* getComponentName(enum Component c) {
    switch (c) {
        case Tobacco:
            return "табак";
        case Paper:
            return "бумага";
        case Matches:
            return "спички";
        default:
            return "неизвестный компонент";
    }
}

void sendMessage(int sock, char *msg) {
    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("Ошибка отправки");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        fprintf(stderr, "Использование: %s <ip сервера> <порт> <id клиента> <компонент (1-3)>\n", argv[0]);
        exit(1);
    }

    char *serverIp = argv[1];
    int serverPort = atoi(argv[2]);

    int clientId = atoi(argv[3]);
    enum Component clientComponent = atoi(argv[4]);

    if (clientId < 1 || clientId > 3 || clientComponent < Tobacco || clientComponent > Matches) {
        fprintf(stderr, "Неверный идентификатор клиента или компонент\n");
        exit(1);
    }

    printf("Клиент %d: Имеет компонент %s\n", clientId, getComponentName(clientComponent));

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Ошибка создания сокета");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIp);
    serverAddress.sin_port = htons(serverPort);

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Ошибка соединения");
        exit(1);
    }

    printf("Клиент %d: Подключен к серверу %s:%d\n", clientId, serverIp, serverPort);

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    sprintf(buffer, "%d %d", clientId, clientComponent);

    sendMessage(clientSocket, buffer);

    printf("Клиент %d: Отправлены идентификатор и компонент на сервер\n", clientId);

    while (1) {

        memset(buffer, 0, MAX_BUFFER);

        if (recv(clientSocket, buffer, MAX_BUFFER - 1, 0) < 0) {
            perror("Ошибка приема");
            exit(1);
        }

        printf("Клиент %d: Получено сообщение от сервера: %s\n", clientId, buffer);

        enum Component component1, component2;

        if (sscanf(buffer, "%d %d", &component1, &component2) != 2 || component1 < Tobacco || component1 > Matches || component2 < Tobacco || component2 > Matches) {
            fprintf(stderr, "Клиент %d: Неверный формат сообщения от сервера\n", clientId);
            continue;
        }

        printf("Клиент %d: Получены два компонента от сервера: %s и %s\n", clientId, getComponentName(component1), getComponentName(component2));

        if (clientComponent != component1 && clientComponent != component2) {

            printf("У клиента %d есть третий компонент\n", clientId);
            sendMessage(clientSocket, "smoking");

            printf("Клиент %d: Отправлено сообщение на сервер: smoking\n", clientId);

            printf("Клиент %d: Берет предметы со стола и курит сигарету\n", clientId);

            sleep(3);

            printf("Клиент %d: Закончил курить сигарету\n", clientId);

        } else {

            printf("Клиент %d: Не имеет третьего компонента\n", clientId);

        }
    }

    close(clientSocket);

    return 0;
}
