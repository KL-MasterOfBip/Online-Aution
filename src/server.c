#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS FD_SETSIZE
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 2048

#define MAX_USERS 100
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 64

typedef struct {
    int socket_fd;
    char buffer[BUFFER_SIZE];
    int length;
    int user_index; /* -1: client chua dang nhap */
} Client;

typedef struct {
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    int is_logged_in;
    int client_fd;
} User;

Client clients[MAX_CLIENTS];
User users[MAX_USERS];
int user_count = 0;

int send_all(int socket_fd, const char *data, int length) {
    int total = 0;

    while (total < length) {
        int sent = send(socket_fd, data + total, (size_t)(length - total), 0);

        if (sent <= 0) {
            if (sent < 0 && errno == EINTR) {
                continue;
            }
            return -1;
        }

        total += sent;
    }

    return total;
}

int send_message(int socket_fd, const char *message) {
    return send_all(socket_fd, message, (int)strlen(message));
}

int find_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }

    return -1;
}

void register_user(int socket_fd,
                   const char *username,
                   const char *password) {
    if (find_user(username) >= 0) {
        send_message(socket_fd, "ERROR Username already exists\n");
        return;
    }

    if (user_count >= MAX_USERS) {
        send_message(socket_fd, "ERROR User database is full\n");
        return;
    }

    User *user = &users[user_count];

    snprintf(user->username, sizeof(user->username), "%s", username);
    snprintf(user->password, sizeof(user->password), "%s", password);

    user->is_logged_in = 0;
    user->client_fd = -1;

    user_count++;

    send_message(socket_fd, "OK Account registered\n");
}

void login_user(int client_index, const char *username, const char *password) {
    Client *client = &clients[client_index];

    if (client->user_index >= 0) {
        send_message(client->socket_fd, "ERROR Already logged in\n");
        return;
    }

    int index = find_user(username);

    if (index < 0 || strcmp(users[index].password, password) != 0) {
        send_message(client->socket_fd, "ERROR Invalid username or password\n");
        return;
    }

    if (users[index].is_logged_in) {
        send_message(client->socket_fd, "ERROR Account already logged in\n");
        return;
    }

    users[index].is_logged_in = 1;
    users[index].client_fd = client->socket_fd;
    client->user_index = index;

    send_message(client->socket_fd, "OK Login successful\n");
}

void logout_user(int client_index) {
    Client *client = &clients[client_index];

    if (client->user_index < 0) {
        send_message(client->socket_fd, "ERROR Not logged in\n");
        return;
    }

    int index = client->user_index;

    if (index < user_count) {
        users[index].is_logged_in = 0;
        users[index].client_fd = -1;
    }

    client->user_index = -1;

    send_message(client->socket_fd, "OK Logout successful\n");
}

int split_tokens(char *command, char *tokens[], int max_tokens) {
    int count = 0;
    char *p = command;

    while (*p != '\0') {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (count >= max_tokens) {
            return max_tokens + 1;
        }

        tokens[count++] = p;

        while (*p != '\0' && !isspace((unsigned char)*p)) {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    return count;
}

void handle_command(int client_index, char *command) {
    int socket_fd = clients[client_index].socket_fd;
    char *tokens[4];
    char response[RESPONSE_SIZE];

    command[strcspn(command, "\r\n")] = '\0';

    int count = split_tokens(command, tokens, 4);

    if (count == 0) {
        send_message(socket_fd, "ERROR Empty command\n");
        return;
    }

    if (strcmp(tokens[0], "PING") == 0 && count == 1) {
        send_message(socket_fd, "OK PONG\n");
    }
    else if (strcmp(tokens[0], "HELP") == 0 && count == 1) {
        send_message(socket_fd,
            "OK Commands: HELP, PING, REGISTER, LOGIN, LOGOUT, QUIT\n");
    }
    else if (strcmp(tokens[0], "REGISTER") == 0 && count == 3) {
        size_t username_length = strlen(tokens[1]);
        size_t password_length = strlen(tokens[2]);

        if (username_length == 0 || username_length >= USERNAME_SIZE ||
            password_length == 0 || password_length >= PASSWORD_SIZE) {
            send_message(socket_fd, "ERROR Username or password is too long or too short\n");
            return;
        }

        register_user(socket_fd, tokens[1], tokens[2]);
    }
    else if (strcmp(tokens[0], "LOGIN") == 0 && count == 3) {
        size_t username_length = strlen(tokens[1]);
        size_t password_length = strlen(tokens[2]);

        if (username_length == 0 || username_length >= USERNAME_SIZE ||
            password_length == 0 || password_length >= PASSWORD_SIZE) {
            send_message(socket_fd, "ERROR Username or password is too long or too short\n");
            return;
        }

        login_user(client_index, tokens[1], tokens[2]);
    }
    else if (strcmp(tokens[0], "LOGOUT") == 0 && count == 1) {
        logout_user(client_index);
    }
    else if (strcmp(tokens[0], "QUIT") == 0 && count == 1) {
        send_message(socket_fd, "OK Goodbye\n");
    }
    else if (strcmp(tokens[0], "PING") == 0 ||
             strcmp(tokens[0], "HELP") == 0 ||
             strcmp(tokens[0], "REGISTER") == 0 ||
             strcmp(tokens[0], "LOGIN") == 0 ||
             strcmp(tokens[0], "LOGOUT") == 0 ||
             strcmp(tokens[0], "QUIT") == 0) {
        send_message(socket_fd, "ERROR Invalid command or arguments\n");
    }
    else {
        snprintf(response, sizeof(response),
                 "ERROR Unknown command: %s\n", tokens[0]);
        send_message(socket_fd, response);
    }
}

void close_client(int index) {
    if (clients[index].socket_fd >= 0) {
        int user_index = clients[index].user_index;

        /* Tu dong dang xuat neu client mat ket noi */
        if (user_index >= 0 && user_index < user_count) {
            users[user_index].is_logged_in = 0;
            users[user_index].client_fd = -1;
        }

        close(clients[index].socket_fd);
        clients[index].socket_fd = -1;
        clients[index].length = 0;
        clients[index].user_index = -1;
    }
}

int main(void) {
    int server_fd;
    int option = 1;
    struct sockaddr_in server_address;

    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = -1;
        clients[i].length = 0;
        clients[i].user_index = -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &option, sizeof(option)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Auction server is running on port %d\n", PORT);

    while (1) {
        fd_set read_fds;
        int max_fd = server_fd;

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = clients[i].socket_fd;

            if (client_fd >= 0) {
                FD_SET(client_fd, &read_fds);
                if (client_fd > max_fd) {
                    max_fd = client_fd;
                }
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd >= 0) {
                int added = 0;

                if (client_fd >= FD_SETSIZE) {
                    close(client_fd);
                    continue;
                }

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket_fd < 0) {
                        clients[i].socket_fd = client_fd;
                        clients[i].length = 0;
                        clients[i].user_index = -1;

                        send_message(client_fd,
                                     "OK Connected to auction server\n");

                        added = 1;
                        printf("Client connected: %d\n", client_fd);
                        break;
                    }
                }

                if (!added) {
                    send_message(client_fd, "ERROR Server is full\n");
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = clients[i].socket_fd;

            if (client_fd < 0 || !FD_ISSET(client_fd, &read_fds)) {
                continue;
            }

            char incoming[BUFFER_SIZE];
            int received = (int)recv(client_fd, incoming, sizeof(incoming), 0);

            if (received <= 0) {
                printf("Client disconnected: %d\n", client_fd);
                close_client(i);
                continue;
            }

            for (int j = 0; j < received; j++) {
                char ch = incoming[j];

                if (ch == '\n') {
                    clients[i].buffer[clients[i].length] = '\0';
                    handle_command(i, clients[i].buffer);

                    int should_close =
                        strcmp(clients[i].buffer, "QUIT") == 0;

                    clients[i].length = 0;

                    if (should_close) {
                        close_client(i);
                        break;
                    }
                }
                else if (clients[i].length < BUFFER_SIZE - 1) {
                    clients[i].buffer[clients[i].length++] = ch;
                }
                else {
                    send_message(client_fd, "ERROR Command too long\n");
                    close_client(i);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        close_client(i);
    }

    close(server_fd);
    return 0;
}