#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <json-c/json.h>

typedef struct
{
    char *name;
    char *port;
} Restaurant;

int restaurants_count = 10;

Restaurant restaurants[10];

// int broadcast_to_customers()
// {
//     char buffer[1024] = {0};
//     // char buffer[1024] = "I'm a restaurant!\n";
//     int sock, broadcast = 1, opt = 1;
//     struct sockaddr_in bc_address;

//     sock = socket(AF_INET, SOCK_DGRAM, 0);
//     setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
//     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     bc_address.sin_family = AF_INET;
//     bc_address.sin_port = htons(8000);
//     bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

//     bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

//     read(0, buffer, 1024);
//     int a = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
//     memset(buffer, 0, 1024);
//     return sock;
// }

int listen_to_broadcasts()
{
    int sock;
    int broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8000);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    return sock;
}

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
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

void fill_restaurants()
{
    for (int i = 0; i < 10; i++)
    {
        Restaurant resi;
        char *name = "resi";
        char *port = "3000";
        resi.name = name;
        resi.port = port;
        restaurants[i] = resi;
    }
}

void show_restaurants()
{
    fill_restaurants();
    printf("\n--------------------\n");
    printf("username/port\n");
    for (int i = 0; i < restaurants_count; i++)
    {
        printf("%s %s\n", restaurants[i].name, restaurants[i].port);
    }
    printf("--------------------\n");
}

int show_menu()
{
    int file_fd = open("recipes.json", O_RDONLY);
    if (file_fd == -1)
    {
        fprintf(stderr, "Error opening JSON file\n");
        return 1;
    }

    char buff[4096]; // Adjust the buffer size according to your needs
    memset(buff, 0, sizeof(buff));
    ssize_t bytes_read = read(file_fd, buff, sizeof(buff) - 1);
    if (bytes_read == -1)
    {
        fprintf(stderr, "Error reading from JSON file\n");
        close(file_fd);
        return 1;
    }

    close(file_fd);

    struct json_object *parsed_json = json_tokener_parse(buff);

    int i = 0;
    json_object_object_foreach(parsed_json, recipe_name, recipe_obj)
    {
        printf("%d- %s\n", i, recipe_name);
        i++;
        struct json_object *ingredients_obj;
        json_object_object_get_ex(parsed_json, recipe_name, &ingredients_obj);

        json_object_object_foreach(ingredients_obj, ingredient_name, quantity_obj)
        {
            int quantity = json_object_get_int(quantity_obj);
            // printf("%s: %d\n", ingredient_name, quantity);
        }
        // printf("\n");
    }

    json_object_put(parsed_json);

    return 0;
}

void timeout_handler(int signum)
{
    printf("no response from restaurant. connection timeout.\n");
}

void order_food()
{
    char food_name[50];
    int res_port;
    printf("name of food: ");
    scanf("%s", food_name);
    printf("restaurant port: ");
    scanf("%d", &res_port);

    printf("food name: %s, restaurant port: %d\n", food_name, res_port);

    printf("waiting for the restaurant's response\n");

    int res_fd;
    struct sockaddr_in server_addr;

    signal(SIGALRM, timeout_handler);
    alarm(10);

    res_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (res_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(res_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(res_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    send(res_fd, food_name, strlen(food_name), 0);
    char response[1024];
    recv(res_fd, response, sizeof(response), 0);
    printf("Received response from restaurant: %s\n", response);

    close(res_fd);
    alarm(0);
}
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void command_detector(char *username, char *command)
{
    if (strcmp(command, "show restaurants\n") == 0)
        show_restaurants();
    else if (strcmp(command, "show menu\n") == 0)
        show_menu();
    else if (strcmp(command, "order food\n") == 0)
        order_food();
}

void sign_in(char *username, char *port)
{
    printf("Enter you preferred username and port please: ");
    scanf("%s %s", username, port);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    char username[50];
    char port[50];
    sign_in(username, port);
    int self_server_fd, new_socket, max_sd;
    char buffer[1024] = {0};

    fd_set master_set, working_set;

    // set up the current server for the customers to connect
    self_server_fd = setupServer(atoi(port));

    FD_ZERO(&master_set);
    FD_SET(self_server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    int broadcast_listen_fd = listen_to_broadcasts();
    FD_SET(broadcast_listen_fd, &master_set);

    max_sd = broadcast_listen_fd;

    char *welcome_message = "Welcome! You're all set.\n";
    write(1, welcome_message, strlen(welcome_message));

    // set up broadcast  -  announce to everyone that there is a new restaurant
    // int broad_sock = broadcast_to_customers();

    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {
                if (i == self_server_fd)
                { // handle new client wanting to connect to restaurant server
                    new_socket = acceptClient(self_server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd = %d\n", new_socket);
                }
                if (i == STDIN_FILENO)
                {
                    char stdin_buffer[1024];
                    fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
                    char *command = stdin_buffer;
                    command_detector(username, command);
                    // send(supplier_fd, std_in_buffer, strlen(std_in_buffer), 0);
                    memset(stdin_buffer, 0, 1024);
                }
                else if (i == broadcast_listen_fd)
                {
                    char message[1024] = {0};
                    recv(broadcast_listen_fd, message, 1024, 0);
                    printf("%s\n", message);
                    memset(message, 0, 1024);
                }
                else
                { // handle a client sending message
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