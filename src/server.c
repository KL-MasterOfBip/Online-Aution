#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 2048

typedef struct {
    int socket_fd;
    char buffer[BUFFER_SIZE];
    int length;
} Client;

Client clients[MAX_CLIENTS];

int send_all(int socket_fd, const char *data, int length) {
    int total = 0;

    while (total < length) {
        int sent = send(socket_fd, data + total, length - total, 0);

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
    return send_all(socket_fd, message, strlen(message));
}

void handle_command(int socket_fd, char *command) {
    char response[RESPONSE_SIZE];

    command[strcspn(command, "\r\n")] = '\0';

    if (strcmp(command, "PING") == 0) {
        send_message(socket_fd, "OK PONG\n");
    } else if (strcmp(command, "HELP") == 0) {
        send_message(socket_fd,
            "OK Commands: HELP, PING, QUIT\n");
    } else if (strcmp(command, "QUIT") == 0) {
        send_message(socket_fd, "OK Goodbye\n");
    } else if (strlen(command) == 0) {
        send_message(socket_fd, "ERROR Empty command\n");
    } else {
        snprintf(response, sizeof(response),
                 "ERROR Unknown command: %s\n", command);
        send_message(socket_fd, response);
    }
}

void close_client(int index) {
    if (clients[index].socket_fd >= 0) {
        close(clients[index].socket_fd);
        clients[index].socket_fd = -1;
        clients[index].length = 0;
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
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &option, sizeof(option));

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
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
            int received = recv(client_fd, incoming, sizeof(incoming), 0);

            if (received <= 0) {
                printf("Client disconnected: %d\n", client_fd);
                close_client(i);
                continue;
            }

            for (int j = 0; j < received; j++) {
                char ch = incoming[j];

                if (ch == '\n') {
                    clients[i].buffer[clients[i].length] = '\0';
                    handle_command(client_fd, clients[i].buffer);

                    int should_close =
                        strcmp(clients[i].buffer, "QUIT") == 0;

                    clients[i].length = 0;

                    if (should_close) {
                        close_client(i);
                        break;
                    }
                } else if (clients[i].length < BUFFER_SIZE - 1) {
                    clients[i].buffer[clients[i].length++] = ch;
                } else {
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
