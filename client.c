/*
Client is a simple user interface to get the data
and send it to the server by socket.
Every client can do the following commands:
    1. Start
    2. Ping 
    3. Stop
*/
// socket libraries


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAXDATALEN 256

int *quit();
void *chat_write(int);
void *chat_read(int);

int n;                        /*variables for socket*/
struct sockaddr_in serv_addr; /* structure to hold server's address */
char buffer[MAXDATALEN];
char buf[10];

int main(int argc, char const *argv[])
{
    pthread_t thr1, thr2;
    int sock = 0;
    struct sockaddr_in serv_addr;

    // TODO: How does socket create?
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // TODO: What is memory cell?
    memset(&serv_addr, '0', sizeof(serv_addr));

    // TODO: What is address family
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    // TODO: Why do we convert IPv4 and IPv6 to binary
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // User commands
    int valread;
    char buffer[1024] = {0};
    char command[50];
    char name[20];

    
    strcpy(name, argv[3]);
    send(sock, name, strlen(name), 0);

    //printf("name=%s\n",name);

    while (1)
    {
        printf("> ");
        gets(command);

        send(sock, command, strlen(command), 0);

        if (strcmp(command, "stop") == 0)
        {
            printf("Disconnected\n");
            break;
        }

        pthread_create(&thr2, NULL, (void *)chat_write, (void *) (intptr_t) sock); //thread for writing
        pthread_create(&thr1, NULL, (void *)chat_read, (void *) (intptr_t) sock);  //thread for reading

        pthread_join(thr2, NULL);
        pthread_join(thr1, NULL);

        /*valread = read(sock, buffer, sizeof(buffer));

        if (valread < 0)
        {
            perror("Reading failed");
            exit(EXIT_FAILURE);
        }*/

    }
    
    return 0;
}

void *chat_read(int sockfd)
{
    signal(SIGINT,(void *)quit);
    while (1)
    {
        n = recv(sockfd, buffer, MAXDATALEN - 1, 0);
        if (n == 0)
        {
            printf("\n==== SERVER HAS BEEN SHUTDOWN ====\n");
            exit(0);
        }

        if (n > 0)
        {
            printf("-> %s", buffer);
            bzero(buffer, MAXDATALEN);
        }
    }
}

void *chat_write(int sockfd)
{
    while (1)
    {
        // printf("%s", buf);
        fgets(buffer, MAXDATALEN - 1, stdin);

        if (strlen(buffer) - 1 > sizeof(buffer))
        {
            printf("buffer size full\t enter within %ld characters\n", sizeof(buffer));
            bzero(buffer, MAXDATALEN);
            //__fpurge(stdin);
        }

        n = send(sockfd, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "/quit", 5) == 0)
            exit(0);

        bzero(buffer, MAXDATALEN);
    }
}

int *quit()
{
    printf("\nType '/quit' TO EXIT\n");
    return 0;
}