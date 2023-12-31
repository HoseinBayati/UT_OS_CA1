#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <termios.h>

typedef struct
{
    char *name;
    char *port;
} Restaurant;

int all_restaurants_length = 0;

Restaurant all_restaurants[100];

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
    {
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

void show_restaurants()
{
    printf("\n--------------------\n");
    printf("username/port\n");

    for (int i = 0; i < all_restaurants_length; i++)
    {
        printf("%s %s\n", all_restaurants[i].name, all_restaurants[i].port);
    }

    printf("--------------------\n\n");
}

int show_menu()
{
    int file_fd = open("recipes.json", O_RDONLY);
    if (file_fd == -1)
    {
        fprintf(stderr, "Error opening JSON file\n");
        return 1;
    }

    char buff[4096];
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
        }
    }

    json_object_put(parsed_json);

    return 0;
}

void timeout_handler(int signum)
{
    printf("- no response from restaurant. connection timeout.\n");
}

void disable_input_buffering()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void restore_input_buffering()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void order_food()
{
    char food_name[50];
    int res_port;
    printf("restaurant port: \n");
    scanf("%d", &res_port);
    printf("name of food: \n");
    scanf("%s", food_name);

    disable_input_buffering();

    char message_to_restaurant[1024];
    char *message_type = "order_food";
    strcpy(message_to_restaurant, message_type);
    strcat(message_to_restaurant, " ");
    strcat(message_to_restaurant, food_name);
    strcat(message_to_restaurant, " ");

    printf("- waiting for the restaurant's response\n");

    int res_fd;
    struct sockaddr_in server_addr;

    signal(SIGALRM, timeout_handler);
    alarm(120);

    res_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (res_fd == -1)
    {
        perror("- Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(res_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(res_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("- Connection failed");
        exit(EXIT_FAILURE);
    }

    send(res_fd, message_to_restaurant, strlen(message_to_restaurant), 0);
    char response[1024] = {0};
    recv(res_fd, response, sizeof(response), 0);
    printf("- Received response from restaurant: %s\n\n", response);

    close(res_fd);
    alarm(0);
    restore_input_buffering();
}

void command_handler(char *username, char *command)
{
    if (strcmp(command, "show restaurants\n") == 0)
        show_restaurants();
    else if (strcmp(command, "show menu\n") == 0)
        show_menu();
    else if (strcmp(command, "order food\n") == 0)
        order_food();
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void remove_restaurant(char *restaurant_info)
{
    char *res_name = strtok(restaurant_info, " ");
    char *res_port = strtok(NULL, "");

    int found = 0;
    for (int i = 0; i < all_restaurants_length; i++)
    {
        if (strcmp(all_restaurants[i].name, res_name) == 0 && strcmp(all_restaurants[i].port, res_port) == 0)
        {
            found = 1;
            free(all_restaurants[i].name);
            free(all_restaurants[i].port);

            all_restaurants[i] = all_restaurants[all_restaurants_length - 1];

            all_restaurants_length--;
            break;
        }
    }

    if (!found)
    {
        printf("- Restaurant '%s' with port '%s' not found.\n\n", res_name, res_port);
    }
    else
    {
        printf("%s Restaurant closed\n\n", all_restaurants[all_restaurants_length - 1].name);
    }
}

bool res_is_duplicate(char *name, char *port)
{
    for (int i = 0; i < all_restaurants_length; i++)
        if (strcmp(all_restaurants[i].name, name) == 0 && strcmp(all_restaurants[i].port, port) == 0)
            return true;

    return false;
}

void add_restaurant(char *restaurant_info)
{
    char *res_name = strtok(restaurant_info, " ");
    char *res_port = strtok(NULL, "");

    if (res_is_duplicate(res_name, res_port))
        return;

    all_restaurants[all_restaurants_length].name = malloc(strlen(res_name) + 1);
    all_restaurants[all_restaurants_length].port = malloc(strlen(res_port) + 1);

    strcpy(all_restaurants[all_restaurants_length].name, res_name);
    strcpy(all_restaurants[all_restaurants_length].port, res_port);

    all_restaurants_length++;
    printf("%s Restaurant opened\n\n", all_restaurants[all_restaurants_length - 1].name);
}

void broadcast_listen_handler(char *message)
{
    char *message_type = strtok(message, " ");
    char *message_info = strtok(NULL, "");

    if (strcmp(message_type, "new_restaurant") == 0)
        add_restaurant(message_info);
    else if (strcmp(message_type, "close_restaurant") == 0)
        remove_restaurant(message_info);
    else
        printf("- broadcast message received: %s\n\n", message);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void sign_in(char *username, char *port)
{
    printf("Enter you preferred username and port please: \n");
    scanf("%s %s", username, port);
}

bool request_restaurants_list()
{
    char message[1024];
    char *message_type = "take_restaurants";
    strcpy(message, message_type);

    broadcast_to_restaurants(message);
    return true;
}

void say_welcome()
{
    char *welcome_message = "Welcome! You're all set.\n\n";
    write(1, welcome_message, strlen(welcome_message));
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    char username[50];
    char self_server_port[50];
    sign_in(username, self_server_port);
    int self_server_fd, new_socket, max_sd;
    char buffer[1024] = {0};

    fd_set master_set, working_set;

    self_server_fd = setupServer(atoi(self_server_port));

    FD_ZERO(&master_set);
    FD_SET(self_server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    int broadcast_listen_fd = listen_to_broadcasts();
    FD_SET(broadcast_listen_fd, &master_set);

    max_sd = broadcast_listen_fd;
    request_restaurants_list();
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
                    printf("New client connected. fd = %d\n\n", new_socket);
                }
                if (i == STDIN_FILENO)
                {
                    char stdin_buffer[1024];
                    fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
                    char *command = stdin_buffer;
                    command_handler(username, command);
                    memset(stdin_buffer, 0, 1024);
                }
                else if (i == broadcast_listen_fd)
                {
                    char broadcast_listen_message[1024] = {0};
                    recv(broadcast_listen_fd, broadcast_listen_message, 1024, 0);
                    broadcast_listen_handler(broadcast_listen_message);
                    memset(broadcast_listen_message, 0, 1024);
                }
                else
                {
                    int bytes_received;
                    bytes_received = recv(i, buffer, 1024, 0);

                    if (bytes_received == 0)
                    {
                        printf("client fd = %d closed\n\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    printf("client %d: %s\n\n", i, buffer);
                    memset(buffer, 0, 1024);
                }
            }
        }
    }

    return 0;
}