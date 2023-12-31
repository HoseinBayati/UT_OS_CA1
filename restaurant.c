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
#include <termios.h>

typedef struct
{
    char *name;
    char *port;
} Supplier;

int all_suppliers_length = 0;

Supplier all_suppliers[100];

typedef struct
{
    char *name;
    char *amount;
} Ingredient;

int all_ingredients_length = 0;

Ingredient all_ingredients[100];

typedef struct
{
    char *food_name;
    int customer_fd;
} Order;

int all_orders_length = 0;

Order all_orders[100];

int broadcast_to_customers(char *message)
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
    {
        printf("- Error in connecting to server\n\n");
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

    broadcast_to_customers(message);
    printf("- Restaurant started working\n- notifications were broadcasted to customers.\n\n");
    return true;
}
bool stop_working(char *self_server_port, char *self_username)
{
    char message[1024];
    char *message_type = "close_restaurant";
    strcpy(message, message_type);
    strcat(message, " ");
    strcat(message, self_username);
    strcat(message, " ");
    strcat(message, self_server_port);

    broadcast_to_customers(message);
    printf("- Restaurant stopped working\n- notifications were broadcasted to customers.\n\n");

    return false;
}
int show_recipes()
{
    int file_fd = open("recipes.json", O_RDONLY);
    if (file_fd == -1)
    {
        fprintf(stderr, "Error opening JSON file\n\n");
        return 1;
    }

    char buff[4096];
    memset(buff, 0, sizeof(buff));
    ssize_t bytes_read = read(file_fd, buff, sizeof(buff) - 1);
    if (bytes_read == -1)
    {
        fprintf(stderr, "Error reading from JSON file\n\n");
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
void show_ingredients()
{
    printf("\n--------------------\n");
    printf("ingredient/amount\n");

    for (int i = 0; i < all_ingredients_length; i++)
    {
        printf("%s %s\n", all_ingredients[i].name, all_ingredients[i].amount);
    }

    printf("--------------------\n\n");
}
void show_suppliers()
{
    printf("\n--------------------\n");
    printf("username/port\n");

    for (int i = 0; i < all_suppliers_length; i++)
    {
        printf("%s %s\n", all_suppliers[i].name, all_suppliers[i].port);
    }

    printf("--------------------\n\n");
}

void timeout_handler(int signum)
{
    printf("- no response from the supplier. connection timeout.\n\n");
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

void request_ingredient()
{
    char ingredient_name[50];
    char ingredient_amount[50];
    int sup_port;
    printf("supplier port: ");
    scanf("%d", &sup_port);
    printf("name of ingredient: ");
    scanf("%s", ingredient_name);
    printf("amount of ingredient: ");
    scanf("%s", ingredient_amount);

    disable_input_buffering();

    char message_to_supplier[1024];
    char *message_type = "request_ingredient";
    strcpy(message_to_supplier, message_type);
    strcat(message_to_supplier, " ");
    strcat(message_to_supplier, ingredient_name);
    strcat(message_to_supplier, " ");
    strcat(message_to_supplier, ingredient_amount);

    printf("- waiting for the supplier's response\n");

    int sup_fd;
    struct sockaddr_in server_addr;

    signal(SIGALRM, timeout_handler);
    alarm(90);

    sup_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sup_fd == -1)
    {
        perror("- Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(sup_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sup_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("- Connection failed");
        exit(EXIT_FAILURE);
    }

    send(sup_fd, message_to_supplier, strlen(message_to_supplier), 0);
    char response[1024];
    recv(sup_fd, response, sizeof(response), 0);
    printf("- Received response from supplier: %s\n\n", response);

    close(sup_fd);
    alarm(0);
    restore_input_buffering();
}

void show_requests()
{
    printf("\n--------------------\n");
    printf("food name/customer\n");

    for (int i = 0; i < all_orders_length; i++)
    {
        printf("%s %d\n", all_orders[i].food_name, all_orders[i].customer_fd);
    }

    printf("--------------------\n\n");
}

void remove_order(int customer_fd)
{
    int found = 0;
    for (int i = 0; i < all_orders_length; i++)
    {
        if (all_orders[i].customer_fd == customer_fd)
        {
            found = 1;
            free(all_orders[i].food_name);

            all_orders[i] = all_orders[all_orders_length - 1];

            all_orders_length--;
            break;
        }
    }

    if (!found)
    {
        printf("- orders from customer: '%d' not found.\n\n", customer_fd);
    }
    else
    {
        printf("%d removed from orders\n\n", all_orders[all_orders_length - 1].customer_fd);
    }
}

void answer_request()
{
    int customer_fd;
    char answer[3];

    printf("customer fd? \n");
    scanf("%d", &customer_fd);
    printf("your answer(yes/no)? \n");
    scanf("%s", answer);

    send(customer_fd, answer, strlen(answer), 0);

    if (strcmp(answer, "yes") == 0)
    {
        remove_order(customer_fd);
    }
}

void show_sales_history()
{
    printf("show sales history\n");
}

void command_handler(char *username, char *command, char *self_username, char *self_server_port, bool *working)
{
    if (strcmp(command, "start working\n") == 0)
        *working = start_working(self_server_port, self_username);
    else if (*working)
    {
        if (strcmp(command, "break\n") == 0)
            *working = stop_working(self_server_port, self_username);
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
        printf("- restaurant is currently closed. \n\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void add_ingredient(char *ing_name, char *ing_amount)
{
    all_ingredients[all_ingredients_length].name = malloc(strlen(ing_name) + 1);
    all_ingredients[all_ingredients_length].amount = malloc(strlen(ing_amount) + 1);

    strcpy(all_ingredients[all_ingredients_length].name, ing_name);
    strcpy(all_ingredients[all_ingredients_length].amount, ing_amount);

    all_ingredients_length++;
}

void remove_supplier(char *supplier_info)
{
    char *res_name = strtok(supplier_info, " ");
    char *res_port = strtok(NULL, "");

    int found = 0;
    for (int i = 0; i < all_suppliers_length; i++)
    {
        if (strcmp(all_suppliers[i].name, res_name) == 0 && strcmp(all_suppliers[i].port, res_port) == 0)
        {
            found = 1;
            free(all_suppliers[i].name);
            free(all_suppliers[i].port);

            all_suppliers[i] = all_suppliers[all_suppliers_length - 1];

            all_suppliers_length--;
            break;
        }
    }

    if (!found)
    {
        printf("- Restaurant '%s' with port '%s' not found.\n\n", res_name, res_port);
    }
    else
    {
        printf("%s removed from suppliers\n\n", all_suppliers[all_suppliers_length - 1].name);
    }
}

bool sup_is_duplicate(char *name, char *port)
{
    for (int i = 0; i < all_suppliers_length; i++)
        if (strcmp(all_suppliers[i].name, name) == 0 && strcmp(all_suppliers[i].port, port) == 0)
            return true;

    return false;
}

void add_supplier(char *supplier_info)
{
    char *sup_name = strtok(supplier_info, " ");
    char *sup_port = strtok(NULL, "");

    if (sup_is_duplicate(sup_name, sup_port))
        return;

    all_suppliers[all_suppliers_length].name = malloc(strlen(sup_name) + 1);
    all_suppliers[all_suppliers_length].port = malloc(strlen(sup_port) + 1);

    strcpy(all_suppliers[all_suppliers_length].name, sup_name);
    strcpy(all_suppliers[all_suppliers_length].port, sup_port);

    all_suppliers_length++;
    printf("%s Supplier added\n\n", all_suppliers[all_suppliers_length - 1].name);
}

void announce_working(char *self_server_port, char *self_username)
{
    char message[1024];
    char *message_type = "new_restaurant";
    strcpy(message, message_type);
    strcat(message, " ");
    strcat(message, self_username);
    strcat(message, " ");
    strcat(message, self_server_port);

    broadcast_to_customers(message);
    printf("- broadcasted working state\n\n");
}

void broadcast_listen_handler(char *message, char *self_username, char *self_server_port, bool *working)
{
    char *message_type = strtok(message, " ");
    char *message_info = strtok(NULL, "");

    if (strcmp(message_type, "new_supplier") == 0)
        add_supplier(message_info);
    else if (strcmp(message_type, "close_supplier") == 0)
        remove_supplier(message_info);
    else if (strcmp(message_type, "take_restaurants") == 0)
    {
        if (*working)
        {
            announce_working(self_server_port, self_username);
        }
    }
    else
        printf("- broadcast message received: %s\n\n", message);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void add_order(char *food_name, int customer_fd)
{
    all_orders[all_orders_length].food_name = malloc(strlen(food_name) + 1);
    all_orders[all_orders_length].customer_fd = customer_fd;

    strcpy(all_orders[all_orders_length].food_name, food_name);

    all_orders_length++;
}

void order_food_handler(char *order_info, int client_fd)
{
    char *food_name = strtok(order_info, "");

    add_order(food_name, client_fd);
    printf("- added order: %s, from customer: %d, to orders queue\n\n", all_orders[all_orders_length - 1].food_name, all_orders[all_orders_length - 1].customer_fd);
}

void request_handler(int client_fd, char *message)
{
    char *message_type = strtok(message, " ");
    char *message_info = strtok(NULL, "");

    if (strcmp(message_type, "order_food") == 0)
        order_food_handler(message_info, client_fd);
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
    char self_username[50];
    char self_server_port[50];
    sign_in(self_username, self_server_port);

    int self_server_fd, new_socket, max_sd, supplier_fd;
    char buffer[1024] = {0};

    fd_set master_set, working_set;

    self_server_fd = setupServer(atoi(self_server_port));

    FD_ZERO(&master_set);
    FD_SET(self_server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    int broadcast_listen_fd = listen_to_broadcasts();
    FD_SET(broadcast_listen_fd, &master_set);

    max_sd = broadcast_listen_fd;
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
                    printf("- New client connected. fd = %d\n\n", new_socket);
                }
                else if (i == STDIN_FILENO)
                {
                    char std_in_buffer[1024];
                    fgets(std_in_buffer, sizeof(std_in_buffer), stdin);
                    char *command = std_in_buffer;
                    command_handler(self_username, command, self_username, self_server_port, &working);
                    memset(std_in_buffer, 0, 1024);
                }
                else if (i == broadcast_listen_fd)
                {
                    char broadcast_listen_message[1024] = {0};
                    recv(broadcast_listen_fd, broadcast_listen_message, 1024, 0);
                    broadcast_listen_handler(broadcast_listen_message, self_username, self_server_port, &working);
                    memset(broadcast_listen_message, 0, 1024);
                }
                else
                {
                    int bytes_received;
                    char request_buffer[1024] = {0};

                    bytes_received = recv(i, request_buffer, 1024, 0);

                    if (bytes_received == 0)
                    {
                        printf("client fd = %d closed\n\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    printf("client %d: %s\n\n", i, request_buffer);
                    if (bytes_received > 0)
                        request_handler(i, request_buffer);
                    memset(request_buffer, 0, 1024);
                }
            }
        }
    }

    return 0;
}