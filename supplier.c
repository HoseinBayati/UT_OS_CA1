#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>

void broadcast_to_restaurants(char *message)
{
    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8080);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    int a = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
    memset(message, 0, 1024);
}

int setupServer(int port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);

    return client_fd;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

bool start_working(char *self_server_port, char *self_username)
{
    char message[1024];
    char *message_type = "new_supplier";
    strcpy(message, message_type);
    strcat(message, " ");
    strcat(message, self_username);
    strcat(message, " ");
    strcat(message, self_server_port);

    broadcast_to_restaurants(message);
    return true;
}

bool stop_working(char *self_server_port, char *self_username)
{
    char message[1024];
    char *message_type = "close_supplier";
    strcpy(message, message_type);
    strcat(message, " ");
    strcat(message, self_username);
    strcat(message, " ");
    strcat(message, self_server_port);

    broadcast_to_restaurants(message);
    return false;
}

void answer_request()
{
    printf("answer request\n");
}

void command_handler(char *username, char *command, char *self_username, char *self_server_port, bool *working)
{
    if (strcmp(command, "start working\n") == 0)
        *working = start_working(self_server_port, self_username);
    else if (*working)
    {
        if (strcmp(command, "break\n") == 0)
            *working = stop_working(self_server_port, self_username);
        else if (strcmp(command, "answer request\n") == 0)
            answer_request();
    }
    else
    {
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void sign_in(char *username, char *port)
{
    printf("Enter you preferred username and port please: ");
    scanf("%s %s", username, port);
}

void say_welcome()
{
    char *welcome_message = "Welcome! You're all set.\n";
    write(1, welcome_message, strlen(welcome_message));
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    char self_username[50];
    char self_server_port[50];
    sign_in(self_username, self_server_port);

    int self_server_fd, new_socket, max_sd;
    char buffer[1024] = {0};
    fd_set master_set, working_set;

    self_server_fd = setupServer(atoi(self_server_port));

    FD_ZERO(&master_set);
    max_sd = self_server_fd;
    FD_SET(self_server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    bool working = false;

    say_welcome();

    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {

                if (i == self_server_fd)
                {
                    new_socket = acceptClient(self_server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd = %d\n", new_socket);
                }
                else if (i == STDIN_FILENO)
                {
                    char std_in_buffer[1024];
                    fgets(std_in_buffer, sizeof(std_in_buffer), stdin);
                    char *command = std_in_buffer;
                    command_handler(self_username, command, self_username, self_server_port, &working);
                    memset(std_in_buffer, 0, 1024);
                }
                else
                {
                    int bytes_received;
                    bytes_received = recv(i, buffer, 1024, 0);

                    if (bytes_received == 0)
                    {
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    printf("client %d: %s\n", i, buffer);
                    memset(buffer, 0, 1024);
                }
            }
        }
    }

    return 0;
}