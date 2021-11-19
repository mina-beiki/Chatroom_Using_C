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
#define MAXGROUP 10    // max number of groups

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
int next_space(char *str);

char username[10];
User users[MAX_USERS] = {0};
int user_tail = 0;
User groups[MAX_GROUPS][MAX_USERS] = {0};
int group_tail[MAX_USERS] = {0};

void *client_handler(void *arguments)
{
    char buffer[MAXDATALEN], uname[10]; /* buffer for string the server sends */
    User *args = arguments;
    int my_port = args->port; /*socket variable passed as arg*/
    char *strp;
    char *msg = (char *)malloc(MAXDATALEN);
    int msglen;
    int x;
    strcpy(uname, args->username);

    while (1)
    {
        
        bzero(buffer, 256);

        /* Client quits */
        if (strncmp(buffer, "quit", 4) == 0)
        {
            printf("** %d: %s left chat. Deleting from lists. **\n\n", my_port, uname);

            delete_list(my_port, users, &user_tail);
            for (int i = 0; i < MAXGROUP; i++)
            {
                delete_list(my_port, groups[i], &group_tail[i]);
            }

            display_list(users, user_tail);

            close(my_port);
            free(msg);
        }
        else if (strncmp(buffer, "join", 4) == 0)
        {
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strcpy(group_id_str, buffer + 6);
            int group_id = atoi(group_id_str);
            printf("** %d: %s joined group number %d. **\n\n", my_port, uname, group_id);

            insert_list(my_port, uname, groups[group_id], &group_tail[group_id]);
        }
        else if (strncmp(buffer, "leave", 5) == 0)
        {
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strcpy(group_id_str, buffer + 7);
            int group_id = atoi(group_id_str);
            printf("** %d: %s left group number %d. **\n\n", my_port, uname, group_id);

            delete_list(my_port, groups[group_id], &group_tail[group_id]);
        }
        else if (strncmp(buffer, "send", 4) == 0)
        {
             int space_pos = next_space(buffer + 6);
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strncpy(group_id_str, buffer + 6, space_pos);
            int group_id = atoi(group_id_str);

            if (search_list(my_port, groups[group_id], group_tail[group_id]) == -1)
            {
                continue;
            }

            printf("%s %s\n", uname, buffer);
            strcpy(msg, uname);
            x = strlen(msg);
            strp = msg;
            strp += x;
            strcat(strp, buffer + 7 + space_pos);
            msglen = strlen(msg);

            for (int i = 0; i < group_tail[group_id]; i++)
            {
                if (groups[group_id][i].port != my_port)
                    send(groups[group_id][i].port, msg, msglen, 0);
            }

            bzero(msg, MAXDATALEN);
        }
        //display_list(users, user_tail);
    }
}

int main(int argc, char const *argv[])
{
    pthread_t thr;
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

         /* getting username */
        bzero(username, 10);
        if (recv(client_socket, username, sizeof(username), 0) > 0)
            ;
        username[strlen(username) - 1] = ':';

        insert_list(client_socket, username, users, &user_tail); 

        // Using thread to handle the client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void *)&client_socket);

        User args; 
        args.port = client_socket;
        strcpy(args.username, username);

        pthread_create(&thr, NULL, client_handler, (void *)&args); 
        pthread_detach(thr);
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

int next_space(char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        if (str[i] == ' ')
        {
            return i;
        }
        i++;
    }
    return -1;
}