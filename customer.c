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
#include <json-c/json.h>

typedef struct
{
    char *name;
    char *port;
} Restaurant;

int restaurants_count = 10;

Restaurant restaurants[10];

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
    FILE *fp = fopen("recipes.json", "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening JSON file\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size);
    fread(json_data, 1, file_size, fp);
    fclose(fp);

    struct json_object *parsed_json = json_tokener_parse(json_data);
    free(json_data);

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

void order_food()
{
    printf("order food\n");
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
    printf("\nWelcome %s!\nYou are registered as a customer now  :))\n\n", username);
    return username;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    int fd, sock;
    char buff[1024] = {0};
    // char *username = sign_in();
    char *username = "ali";

    fd = connectServer(8080);

    // setup broadcast (take from restaurants)
    int broadcast = 1, opt = 1;
    char buffer[1024] = {0};
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8000);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    fd_set read_fds;
    int max_fd;

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);    // Add stdin (user input) to the set
        FD_SET(sock, &read_fds); // Add broadcasting socket to the set
        max_fd = (sock > 0) ? sock : 0;

        // Wait for activity on any of the monitored file descriptors
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        // Check if there is user input to send
        if (FD_ISSET(0, &read_fds))
        {
            memset(buff, 0, sizeof(buff));
            read(0, buff, sizeof(buff) - 1);
            char *command = buff;
            command_detector(username, command);
            // send(fd, buff, strlen(buff), 0);
        }

        // Check if there is a broadcast message to receive
        if (FD_ISSET(sock, &read_fds))
        {
            memset(buffer, 0, 1024);
            recv(sock, buffer, 1024, 0);
            printf("%s\n", buffer);
        }
    }

    close(fd);
    close(sock);

    return 0;
}