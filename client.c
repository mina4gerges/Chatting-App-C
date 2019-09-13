#include <stdio.h>
#include <stdlib.h>//marcos atoi
#include <string.h>//strcmp, strlen
#include <signal.h>//signal
#include <unistd.h>//sleep, close, write
#include <sys/socket.h>
#include <arpa/inet.h>//int_addr
#include <pthread.h>

// Global variables
int flag = 0;
int sockfd = 0;
char name[32];

void str_overwrite_stdout()//func to print > in new line and clear the output strea,
{
    printf("%s", "> ");
    fflush(stdout);//to clear or fluch the output stream (bcz value it us stored in the buffer);
}

void str_rmv_newline(char *arr, int length)
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

void catch_ctrl_c_and_exit(int sig)//function for the SIGNAL 
{
    flag = 1;
}

void send_msg_handler()
{
    char message[2048];
    char buffer[2048 + 32];

    while (1)
    {
        str_overwrite_stdout();
        fgets(message, 2048, stdin);//get msg from user
        str_rmv_newline(message, 2048);//remove \n

        if (strcmp(message, "exit") == 0)//if user enter exit --> stop
        {
            break;
        }
        else
        {
            sprintf(buffer, "%s: %s\n", name, message);
            send(sockfd, buffer, strlen(buffer), 0);
        }

        bzero(message, 2048);//empty message
        bzero(buffer, 2048 + 32);//empty buffer
    }
    flag = 1;
}

void recv_msg_handler()
{
    char message[2048];
    while (1)
    {
        int receive = recv(sockfd, message, 2048, 0);
        if (receive > 0)
        {
            printf("%s", message);
            str_overwrite_stdout();
        }
        else if (receive == 0)
        {
            break;
        }
        bzero(message, 2048);//empty message
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    signal(SIGINT, catch_ctrl_c_and_exit);//if user click on ctrl+c to stop the program (exit)

    printf("Please enter your name: ");
    fgets(name, 32, stdin);//get input from the user
    str_rmv_newline(name, strlen(name));

    if (strlen(name) > 32)//32 bcz we defined size of nam is 32 byte 
    {
        printf("Name must be less than 30\n");
        return 0;
    }

    struct sockaddr_in server_addr;

    /* Socket settings */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connect to Server
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1)
    {
        printf("ERROR: connect\n");
        return 0;
    }

    // Send name
    send(sockfd, name, 32, 0);

    printf("Start Chatting \n");

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        return 0;
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        return 0;
    }

    while (1)
    {
        if (flag)
        {
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd);

    return 1;
}