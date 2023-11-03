#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <json-c/json.h>

int broadcast_start_working(char *message)
{
    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8000);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    int a = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));

    memset(message, 0, 1024);
    return sock;
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
    bc_address.sin_port = htons(8080);
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

bool start_working(char *self_server_port, char *self_username)
{
    char message[1024];
    char *message_type = "new_restaurant";
    strcpy(message, message_type);
    strcat(message, " ");
    strcat(message, self_username);
    strcat(message, " ");
    strcat(message, self_server_port);

    broadcast_start_working(message);
    return true;
}
bool stop_working()
{
    printf("work halted!\n");
    return false;
}
void show_ingredients()
{
    printf("show ingredients\n");
}
int show_recipes()
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
            printf("       %s: %d\n", ingredient_name, quantity);
        }
    }

    json_object_put(parsed_json);

    return 0;
}
void show_suppliers()
{
    printf("show suppliers\n");
}
void request_ingredient()
{
    printf("request ingredient\n");
}
void show_requests()
{
    printf("show requests\n");
}
void answer_request()
{
    printf("answer request\n");
}
void show_sales_history()
{
    printf("show sales history\n");
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void command_handler(char *username, char *command, char *self_username, char *self_server_port, bool *working)
{
    if (strcmp(command, "start working\n") == 0)
        *working = start_working(self_server_port, self_username);
    else if (*working)
    {
        if (strcmp(command, "break\n") == 0)
            *working = stop_working();
        else if (strcmp(command, "show ingredients\n") == 0)
            show_ingredients();
        else if (strcmp(command, "show recipes\n") == 0)
            show_recipes();
        else if (strcmp(command, "show suppliers\n") == 0)
            show_suppliers();
        else if (strcmp(command, "request ingredient\n") == 0)
            request_ingredient();
        else if (strcmp(command, "show requests\n") == 0)
            show_requests();
        else if (strcmp(command, "answer request\n") == 0)
            answer_request();
        else if (strcmp(command, "show sales history\n") == 0)
            show_sales_history();
    }
    else
    {
    }
}

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

    int self_server_fd, new_socket, max_sd, supplier_fd;
    char buffer[1024] = {0};

    fd_set master_set, working_set;

    // connect to a supplier  -  connect to the target server
    supplier_fd = connectServer(3001);
    // set up the current server for the customers to connect
    self_server_fd = setupServer(atoi(self_server_port));

    FD_ZERO(&master_set);
    FD_SET(self_server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    // set up broadcast  -  announce to everyone that there is a new restaurant
    // int broad_sock = broadcast_to_customers();

    // setup broadcast (take from suppliers)
    // int suppliers_sock = broad_sock;
    int suppliers_sock = listen_to_broadcasts();
    FD_SET(suppliers_sock, &master_set);
    ////////////////////////////////////////
    max_sd = suppliers_sock;
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
                { // handle new client wanting to connect to restaurant server
                    new_socket = acceptClient(self_server_fd);
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
                    command_handler(self_username, command, self_username, self_server_port, &working);
                    // send(supplier_fd, std_in_buffer, strlen(std_in_buffer), 0);
                    memset(std_in_buffer, 0, 1024);
                }
                else if (i == suppliers_sock)
                {
                    char supplier_message[1024] = {0};
                    recv(suppliers_sock, supplier_message, 1024, 0);
                    printf("%s\n", supplier_message);
                    memset(supplier_message, 0, 1024);
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