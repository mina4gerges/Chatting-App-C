#include <sys/socket.h>
#include <arpa/inet.h>//int_addr
#include <stdio.h>//printf
#include <stdlib.h>//marcos atoi
#include <unistd.h>//sleep, close, write
#include <string.h> //strcmp, strlen
#include <pthread.h>

static int cli_count = 0;
static int uid = 10;

/* Client structure */
 typedef struct //typedef is like new object in java
{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
 } client_t;


client_t *clients[10]; // struct to put the users that are connected

void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
    { // trim \n
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

/* Add clients to queue */
void queue_add(client_t *cl, int cli_count)
{
    clients[cli_count] = cl;
}

/* Remove clients to queue */
void queue_remove(int uid)
{
    for (int i = 0; i < 10; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }
}

/* Send message to all clients except sender */
void send_message(char *s, int uid)
{
    for (int i = 0; i < 10; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid != uid)
            {
                if (write(clients[i]->sockfd, s, strlen(s)) < 0)
                {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }
}

/* Handle all communication with the client */
void *handle_client(void *arg)
{
    char buff_out[2048];
    char name[32];
    int leave_flag = 0;

    client_t *cli = (client_t *)arg;

    // Name
    if (recv(cli->sockfd, name, 32, 0) <= 0) //recv from socket
    {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    }
    else
    {
        strcpy(cli->name, name); //string copy
        sprintf(buff_out, "%s has joined\n", cli->name); //sprintf put  cli->name in buff_out
        printf("%s", buff_out);
        send_message(buff_out, cli->uid);
    }

    bzero(buff_out, 2048);//erase the data by writing zeros(bytes containing '\0')

    while (1)
    {
        if (leave_flag)
        {
            break;
        }

        int receive = recv(cli->sockfd, buff_out, 2048, 0);
        if (receive > 0)
        {
            if (strlen(buff_out) > 0)
            {
                send_message(buff_out, cli->uid);

                str_trim_lf(buff_out, strlen(buff_out));
                printf("%s \n", buff_out);
            }
        }
        else if (receive == 0 || strcmp(buff_out, "exit") == 0)
        {
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            leave_flag = 1;
        }
        else
        {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, 2048);
    }

    /* Delete client from queue and yield thread */
    close(cli->sockfd);//close socket
    queue_remove(cli->uid);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("PLeas insert: %s <port>\n", argv[0]);
        return 0;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0); // (domaine, type, protocol) AF_INET-->IPV4 | SOCK_STREAM --> TCP or SOCK_DREAM -->UDP
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip); //inet_addr convert string pointed to an integer. (we can user INADDR_ANY means all IP available)
    serv_addr.sin_port = htons(port);          //converts from host byte order to network byte order.

    /* Bind */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR: Socket binding failed \n");
        return 0;
    }

    /* Listen */
    if (listen(listenfd, 10) < 0)
    {
        printf("ERROR: Socket listening failed \n");
        return 0;
    }

    printf("Server Is Ready\n");
    while (1)
    {
        cli_count++;
        socklen_t clilen = sizeof(cli_addr);                              //last params used in accept
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen); //creates a new connected socket

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        /* Add client to the queue and fork thread */
        queue_add(cli, cli_count);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);
    }

    return 0;
}