// webServer.cpp : Defines the entry point for the application.
//

#include "webServer.h"
#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 5
#define PORT 5000
#define IP "127.0.0.1"

int num_clients = 0;
int prev_num_clients = 0;
pthread_mutex_t mutex_num_clients;

void* handle_client(void* arg)
{
    int sockfd = *((int*)arg);
    char buffer[256];
    int n;

    // Зчитуємо запит клієнта
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Виводимо отриманий запит
    std::cout << "Message from client: " << buffer << std::endl;

    // Надсилаємо відповідь клієнту
    const char* message = "Server response.";
    n = write(sockfd, message, strlen(message));
    if (n < 0) {
        perror("ERROR writing to socket");
    }

    close(sockfd);

    pthread_mutex_lock(&mutex_num_clients);
    num_clients--;

    if (prev_num_clients != num_clients)
    {
        prev_num_clients = num_clients;
        printf("Active clients: %d\n", num_clients);
    }

    pthread_mutex_unlock(&mutex_num_clients);

    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t thread;
    int client_sockets[MAX_CLIENTS];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = PORT;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);

    printf("Server started\n");

    while (true)
    {
        if (num_clients >= MAX_CLIENTS) {
            std::cerr << "Maximum number of clients reached." << std::endl;
            continue;
        }

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Надсилаємо відповідь клієнту про успішне підключення
        const char* connected_msg = "You are connected to the server.";
        int n = write(newsockfd, connected_msg, strlen(connected_msg));
        if (n < 0) {
            perror("ERROR writing to socket");
        }

        // Зберігаємо сокет клієнта та запускаємо новий потік для його обробки
        client_sockets[num_clients] = newsockfd;
        if (pthread_create(&thread, NULL, handle_client, (void*)&client_sockets[num_clients])) {
            perror("ERROR creating thread");
        }

        pthread_mutex_lock(&mutex_num_clients);
        num_clients++;
        
        if (prev_num_clients != num_clients)
        {
            prev_num_clients = num_clients;
            printf("Active clients: %d\n", num_clients);
        }

        pthread_mutex_unlock(&mutex_num_clients);
    }

    close(sockfd);
    return 0;
}
