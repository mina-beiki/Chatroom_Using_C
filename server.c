/* 
Using sockets to connect to our server.
Our server which will accept clients and give response to them.
Our main input command are: 
    1. Start [name]
    2. Ping 
    3. Stop
Server responses:
    1. Init user
    2. Pong [status]
    3. Release client
*/

// Socket libraries
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//
#include <string.h>
#include <ctype.h>
#include <time.h>
// Thread library
#include <pthread.h>

#define MAX_USERS 100
#define MAX_GROUPS 10
#define MAXDATALEN 256 // max size of messages to be sent

/* Client structure */
typedef struct{
	int port;
	char username[32];
} User;

void insert_list(int port, char *username, User *list, int *tail); /*inserting new client */
int search_list(int port, User *list, int tail);
void delete_list(int port, User *list, int *tail);
void delete_all(User *list, int *tail);
void display_list(const User *list, int tail); /*list all clients connected*/

char username[10];
User users[MAX_USERS] = {0};
int user_tail = 0;
User groups[MAX_GROUPS][MAX_USERS] = {0};
int group_tail[MAX_USERS] = {0};
char buffer[MAXDATALEN];

void *client_handler(void *vargp)
{
    int *temp = (int *)vargp;
    int client_socket = *temp;

    int valread;
    char buffer[1024] = {0};
    char response[1024] = {0};

    clock_t begin = clock();

    /* Client settings */
        valread = read(client_socket, buffer, sizeof(buffer));
		User *user_s = (User *)malloc(sizeof(User));
		User args; 
        args.port = client_socket;
        strcpy(args.username, buffer);

        buffer[valread] = '\0';
        fflush(stdout);
        buffer[0] = '\0';
        response[0] = '\0';
    
    while (1)
    {
        
        valread = read(client_socket, buffer, sizeof(buffer));
        if (valread < 0)
        {
            perror("Empty read");
            exit(EXIT_FAILURE);
        }

        if (strcmp(buffer, "name") == 0)
        {
            printf("Client %d's name: %s\n", client_socket, buffer);
            break;
        }
        
        buffer[valread] = '\0';

        if (strcmp(buffer, "quit") == 0)
        {
            printf("Client %d: disconnected\n", client_socket);
            break;
        }

        if (strcmp(buffer, "join") == 0)
        {
            printf("Client %d: wants to join!\n", client_socket);
            break;
        }

        time_t mytime = time(NULL);
        char * time_str = ctime(&mytime);
        time_str[strlen(time_str)-1] = '\0';

        double time_spent = (double)(clock() - begin) / CLOCKS_PER_SEC * 1000;
        snprintf(response, sizeof(response), "%s Pong %d: %fs", time_str, client_socket, time_spent);

        send(client_socket, response, sizeof(response), 0);
        printf("%s: User %d\n", time_str, client_socket);

        fflush(stdout);
        buffer[0] = '\0';
        response[0] = '\0';
    }
}

int main(int argc, char const *argv[])
{
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // TODO: What is this do
    if (server_fd == 0)
    {
        perror("Socket faild");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));
    const int addrlen = sizeof(address);

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen faild");
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));


    // Accepting client
    while (1)
    {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept faild");
            exit(EXIT_FAILURE);
        }

        printf("Accepted client %s:%d id:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), client_socket);

        // Using thread to handle the client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void *)&client_socket);
    }

    return 0;
}

void insert_list(int port, char *username, User *list, int *tail)
{
    if (search_list(port, list, *tail) != -1)
    {
        return;
    }
    User *temp;
    temp = malloc(sizeof(User));
    if (temp == NULL)
        printf("Out of space!");
    temp->port = port;
    strcpy(temp->username, username);
    list[(*tail)++] = *temp;
}

int search_list(int port, User *list, int tail)
{
    for (int i = 0; i < tail; i++)
    {
        if (list[i].port == port)
            return i;
    }
    return -1;
}

void delete_list(int port, User *list, int *tail)
{
    int ptr = search_list(port, list, *tail);
    if (ptr == -1)
    {
        return;
    }

    for (int i = ptr; i < *tail - 1; i++)
    {
        list[i] = list[i + 1];
    }
    (*tail)--;
}

void display_list(const User *list, int tail)
{
    printf("Current online users:\n");
    if (tail == 0)
    {
        printf("No one is online\n");
        return;
    }

    for (int i = 0; i < tail; i++)
    {
        printf("%d: %s\t", list[i].port, list[i].username);
    }
    printf("\n\n");
}

void delete_all(User *list, int *tail)
{
    *tail = 0;
}