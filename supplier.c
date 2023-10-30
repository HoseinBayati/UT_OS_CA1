#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

void broadcast_to_restaurants()
{
    char buffer[1024] = {0};
    // char buffer[1024] = "I'm a supplier!\n";

    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8000);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    read(0, buffer, 1024);
    int a = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
    memset(buffer, 0, 1024);
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

void answer_request()
{
    printf("answer request\n");
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void command_detector(char *username, char *command)
{
    if (strcmp(command, "answer request\n") == 0)
        printf("answer request\n");
    else
        printf("what's this jibberish\n");
}

char *sign_in()
{
    char *username = (char *)malloc(1024 * sizeof(char));
    if (username == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    printf("Please enter your username: ");
    scanf("%s", username);
    printf("\nWelcome %s!\nYou are registered as a supplier now  :)\n\n", username);
    return username;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, max_sd;
    char buffer[1024] = {0};
    fd_set master_set, working_set;

    // char *username = sign_in();
    char *username = "supplier1";

    server_fd = setupServer(3000);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    write(1, "Server is running\n", 18);

    // set up broadcast  -  announce to everyone that there is a new supplier
    broadcast_to_restaurants();

    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {

                if (i == server_fd)
                { // new clinet
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd = %d\n", new_socket);
                }
                if (i == STDIN_FILENO)
                { // handle an input from the command line (like sending it to a supplier or anything)
                    char std_in_buffer[1024];
                    fgets(std_in_buffer, sizeof(std_in_buffer), stdin);
                    char *command = std_in_buffer;
                    command_detector(username, command);
                    memset(std_in_buffer, 0, 1024);
                }
                else
                { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i, buffer, 1024, 0);

                    if (bytes_received == 0)
                    { // EOF
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