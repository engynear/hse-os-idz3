#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_CLIENTS 3
#define MAX_BUFFER 256

enum Component
{
    Tobacco = 1,
    Paper = 2,
    Matches = 3
};

struct Client
{
    int id;
    int socket;
    enum Component component;
};

void generateComponents(enum Component *c1, enum Component *c2)
{
    int r = rand() % 3 + 1;
    switch (r)
    {
    case 1:
        *c1 = Paper;
        *c2 = Matches;
        break;
    case 2:
        *c1 = Tobacco;
        *c2 = Matches;
        break;
    case 3:
        *c1 = Tobacco;
        *c2 = Paper;
        break;
    }
}

const char *getComponentName(enum Component c)
{
    switch (c)
    {
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

int hasThirdComponent(struct Client *cl, enum Component c1, enum Component c2)
{
    return (cl->component != c1 && cl->component != c2);
}

void sendMessage(int sock, char *msg)
{
    if (send(sock, msg, strlen(msg), 0) < 0)
    {
        perror("Ошибка при отправке сообщения");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <порт>\n", argv[0]);
        exit(1);
    }

    srand(time(NULL));

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0)
    {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    int newSocket;
    int port = atoi(argv[1]);
    struct sockaddr_in serverAddress;
    int option = 1;
    pthread_t threadId;

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)))
    {
        perror("Ошибка при установке опций сокета");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Ошибка при привязке сокета");
        exit(1);
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0)
    {
        perror("Ошибка при прослушивании сокета");
        exit(1);
    }

    printf("Сервер: Запущен на порте %d\n", port);

    struct Client clients[MAX_CLIENTS];
    int numClients = 0;

    while (numClients < MAX_CLIENTS)
    {
        struct sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);

        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength);
        if (clientSocket < 0)
        {
            perror("Ошибка при принятии соединения");
            exit(1);
        }

        printf("Сервер: Принято входящее подключение %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        char buffer[MAX_BUFFER];
        memset(buffer, 0, MAX_BUFFER);

        if (recv(clientSocket, buffer, MAX_BUFFER - 1, 0) < 0)
        {
            perror("Ошибка при чтении сообщения от клиента");
            exit(1);
        }

        printf("Сервер: Получено сообщение от клиента: %s\n", buffer);

        int id;
        enum Component comp;

        if (sscanf(buffer, "%d %u", &id, &comp) != 2 || id < 1 || id > MAX_CLIENTS || comp < Tobacco || comp > Matches)
        {
            fprintf(stderr, "Сервер: Получены некорректные данные от клиента\n");
            close(clientSocket);
            continue;
        }

        printf("Сервер: Получены данные от клиента: id=%d, component=%s\n", id, getComponentName(comp));

        clients[id - 1].id = id;
        clients[id - 1].socket = clientSocket;
        clients[id - 1].component = comp;

        numClients++;

        printf("Сервер: Клиент %d добавлен\n", id);
    }

    printf("Сервер: Все клиенты подключены\n");

    while (1)
    {
        enum Component component1, component2;
        generateComponents(&component1, &component2);

        printf("Сервер: Сгенерированы компоненты: %s и %s\n", getComponentName(component1), getComponentName(component2));

        char buffer[MAX_BUFFER];
        memset(buffer, 0, MAX_BUFFER);
        sprintf(buffer, "%d %d", component1, component2);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            sendMessage(clients[i].socket, buffer);
        }

        printf("Сервер: Отправлены компоненты клиентам\n");

        int smokerFound = 0;
        int smokerId;

        while (!smokerFound)
        {
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                memset(buffer, 0, MAX_BUFFER);

                if (recv(clients[i].socket, buffer, MAX_BUFFER - 1, 0) < 0)
                {
                    perror("recv error");
                    exit(1);
                }

                printf("Сервер: Получено сообщение от клиента %d: %s\n", clients[i].id, buffer);

                if (strstr(buffer, "smoking") != NULL)
                {
                    smokerFound = 1;
                    smokerId = clients[i].id;
                    break;
                }
            }
            if (smokerFound)
            {
                break;
            }
            sleep(1);
        }

        printf("Сервер: Клиент %d получил компоненты и начал курить\n", smokerId);

        sleep(3);

        printf("Сервер: Клиент %d закончил курить\n", smokerId);
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        close(clients[i].socket);
    }
    close(serverSocket);

    return 0;
}
