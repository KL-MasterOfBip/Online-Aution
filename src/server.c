#define _POSIX_C_SOURCE 200809L
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
#include <stdarg.h>
#include <time.h>

#ifndef PORT
#define PORT 8080
#endif
#define MAX_CLIENTS FD_SETSIZE
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 2048

#define MAX_USERS 100
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 64

#define MAX_ROOMS 100
#define ROOM_NAME_SIZE 50

typedef struct {
    int socket_fd;
    char buffer[BUFFER_SIZE];
    int length;
    int user_index;       /* -1: client chua dang nhap */
    int current_room_id;  /* -1: client chua tham gia phong nao */
} Client;

typedef struct {
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    int is_logged_in;
    int client_fd;
} User;

typedef struct {
    int room_id;
    char room_name[ROOM_NAME_SIZE];
    char owner[USERNAME_SIZE];
    int member_count;
    int is_active;
} AuctionRoom;

Client clients[MAX_CLIENTS];
User users[MAX_USERS];
AuctionRoom rooms[MAX_ROOMS];
int user_count = 0;
int next_room_id = 1;

#define LOG_FILE "auction_server.log"

/* Ghi log co timestamp; tuyet doi khong ghi password vao file log. */
void log_event(const char *format, ...) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Cannot open log file");
        return;
    }

    time_t now = time(NULL);
    struct tm time_info;
    char timestamp[32] = "unknown-time";

    if (localtime_r(&now, &time_info) != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
                 &time_info);
    }

    fprintf(log_file, "[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fputc('\n', log_file);
    fclose(log_file);
}

int send_all(int socket_fd, const char *data, int length) {
    int total = 0;

    while (total < length) {
        int sent = (int)send(socket_fd, data + total,
                             (size_t)(length - total), 0);

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

int find_room_by_id(int room_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == room_id) {
            return i;
        }
    }

    return -1;
}

int find_room_by_name(const char *room_name) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active &&
            strcmp(rooms[i].room_name, room_name) == 0) {
            return i;
        }
    }

    return -1;
}

/* Cap nhat so thanh vien va trang thai phong cua client. */
void remove_client_from_room(int client_index) {
    Client *client = &clients[client_index];

    if (client->current_room_id < 0) {
        return;
    }

    int room_index = find_room_by_id(client->current_room_id);

    if (room_index >= 0 && rooms[room_index].member_count > 0) {
        rooms[room_index].member_count--;
    }

    client->current_room_id = -1;
}

void register_user(int client_index,
                   const char *username,
                   const char *password) {
    Client *client = &clients[client_index];
    int socket_fd = client->socket_fd;

    /* Chi guest moi duoc dang ky tai khoan. */
    if (client->user_index >= 0) {
        send_message(socket_fd, "ERROR Cannot register while logged in\n");
        return;
    }

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

    log_event("REGISTER_SUCCESS username=%s", username);
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
        send_message(client->socket_fd,
                     "ERROR Invalid username or password\n");
        return;
    }

    if (users[index].is_logged_in) {
        send_message(client->socket_fd,
                     "ERROR Account already logged in\n");
        return;
    }

    users[index].is_logged_in = 1;
    users[index].client_fd = client->socket_fd;
    client->user_index = index;

    log_event("LOGIN_SUCCESS username=%s fd=%d", username, client->socket_fd);
    send_message(client->socket_fd, "OK Login successful\n");
}

void logout_user(int client_index) {
    Client *client = &clients[client_index];

    if (client->user_index < 0) {
        send_message(client->socket_fd, "ERROR Not logged in\n");
        return;
    }

    /* Dang xuat dong thoi roi khoi phong neu dang tham gia. */
    const char *username = users[client->user_index].username;
    int old_room_id = client->current_room_id;
    remove_client_from_room(client_index);
    log_event("LOGOUT username=%s fd=%d previous_room=%d",
              username, client->socket_fd, old_room_id);

    int index = client->user_index;

    if (index < user_count) {
        users[index].is_logged_in = 0;
        users[index].client_fd = -1;
    }

    client->user_index = -1;
    send_message(client->socket_fd, "OK Logout successful\n");
}

void create_room(int client_index, const char *room_name) {
    Client *client = &clients[client_index];

    if (client->user_index < 0) {
        send_message(client->socket_fd, "ERROR Not logged in\n");
        return;
    }

    if (find_room_by_name(room_name) >= 0) {
        send_message(client->socket_fd, "ERROR Room already exists\n");
        return;
    }

    int slot = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].is_active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        send_message(client->socket_fd, "ERROR Room database is full\n");
        return;
    }

    AuctionRoom *room = &rooms[slot];
    room->room_id = next_room_id++;
    snprintf(room->room_name, sizeof(room->room_name), "%s", room_name);
    snprintf(room->owner, sizeof(room->owner), "%s",
             users[client->user_index].username);
    room->member_count = 0;
    room->is_active = 1;

    log_event("ROOM_CREATED id=%d name=%s owner=%s",
              room->room_id, room->room_name, room->owner);

    char response[128];
    snprintf(response, sizeof(response), "OK ROOM_CREATED %d\n", room->room_id);
    send_message(client->socket_fd, response);
}

void list_rooms(int client_index) {
    int socket_fd = clients[client_index].socket_fd;
    int room_count = 0;

    send_message(socket_fd, "OK ROOM_LIST\n");

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active) {
            char response[160];
            snprintf(response, sizeof(response),
                     "ROOM %d %s OWNER %s MEMBERS %d\n",
                     rooms[i].room_id,
                     rooms[i].room_name,
                     rooms[i].owner,
                     rooms[i].member_count);
            if (send_message(socket_fd, response) < 0) {
                return;
            }
            room_count++;
        }
    }

    if (room_count == 0) {
        send_message(socket_fd, "NO_ROOMS\n");
    }

    send_message(socket_fd, "END_ROOM_LIST\n");
}

void join_room(int client_index, int room_id) {
    Client *client = &clients[client_index];

    if (client->user_index < 0) {
        send_message(client->socket_fd, "ERROR Not logged in\n");
        return;
    }

    if (client->current_room_id >= 0) {
        send_message(client->socket_fd, "ERROR Already in a room\n");
        return;
    }

    int room_index = find_room_by_id(room_id);
    if (room_index < 0) {
        send_message(client->socket_fd, "ERROR Room not found\n");
        return;
    }

    client->current_room_id = room_id;
    rooms[room_index].member_count++;

    log_event("ROOM_JOIN username=%s fd=%d room_id=%d members=%d",
              users[client->user_index].username, client->socket_fd,
              room_id, rooms[room_index].member_count);

    char response[128];
    snprintf(response, sizeof(response), "OK JOINED_ROOM %d\n", room_id);
    send_message(client->socket_fd, response);
}

void leave_room(int client_index) {
    Client *client = &clients[client_index];

    if (client->user_index < 0) {
        send_message(client->socket_fd, "ERROR Not logged in\n");
        return;
    }

    if (client->current_room_id < 0) {
        send_message(client->socket_fd, "ERROR Not in a room\n");
        return;
    }

    int old_room_id = client->current_room_id;
    const char *username = users[client->user_index].username;
    remove_client_from_room(client_index);
    log_event("ROOM_LEAVE username=%s fd=%d room_id=%d",
              username, client->socket_fd, old_room_id);
    send_message(client->socket_fd, "OK LEFT_ROOM\n");
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
            "OK Commands: HELP, PING, REGISTER, LOGIN, LOGOUT, "
            "CREATE_ROOM, LIST_ROOMS, JOIN_ROOM, LEAVE_ROOM, QUIT\n");
    }
    else if (strcmp(tokens[0], "REGISTER") == 0 && count == 3) {
        size_t username_length = strlen(tokens[1]);
        size_t password_length = strlen(tokens[2]);

        if (username_length == 0 || username_length >= USERNAME_SIZE ||
            password_length == 0 || password_length >= PASSWORD_SIZE) {
            send_message(socket_fd,
                         "ERROR Username or password is too long or too short\n");
            return;
        }

        register_user(client_index, tokens[1], tokens[2]);
    }
    else if (strcmp(tokens[0], "LOGIN") == 0 && count == 3) {
        size_t username_length = strlen(tokens[1]);
        size_t password_length = strlen(tokens[2]);

        if (username_length == 0 || username_length >= USERNAME_SIZE ||
            password_length == 0 || password_length >= PASSWORD_SIZE) {
            send_message(socket_fd,
                         "ERROR Username or password is too long or too short\n");
            return;
        }

        login_user(client_index, tokens[1], tokens[2]);
    }
    else if (strcmp(tokens[0], "LOGOUT") == 0 && count == 1) {
        logout_user(client_index);
    }
    else if (strcmp(tokens[0], "CREATE_ROOM") == 0 && count == 2) {
        if (strlen(tokens[1]) >= ROOM_NAME_SIZE) {
            send_message(socket_fd, "ERROR Room name is too long\n");
            return;
        }
        create_room(client_index, tokens[1]);
    }
    else if (strcmp(tokens[0], "LIST_ROOMS") == 0 && count == 1) {
        list_rooms(client_index);
    }
    else if (strcmp(tokens[0], "JOIN_ROOM") == 0 && count == 2) {
        char *end = NULL;
        errno = 0;
        long room_id = strtol(tokens[1], &end, 10);

        if (errno != 0 || tokens[1][0] == '\0' || *end != '\0' ||
            room_id <= 0 || room_id > 2147483647L) {
            send_message(socket_fd, "ERROR Invalid room ID\n");
            return;
        }

        join_room(client_index, (int)room_id);
    }
    else if (strcmp(tokens[0], "LEAVE_ROOM") == 0 && count == 1) {
        leave_room(client_index);
    }
    else if (strcmp(tokens[0], "QUIT") == 0 && count == 1) {
        send_message(socket_fd, "OK Goodbye\n");
    }
    else if (strcmp(tokens[0], "PING") == 0 ||
             strcmp(tokens[0], "HELP") == 0 ||
             strcmp(tokens[0], "REGISTER") == 0 ||
             strcmp(tokens[0], "LOGIN") == 0 ||
             strcmp(tokens[0], "LOGOUT") == 0 ||
             strcmp(tokens[0], "CREATE_ROOM") == 0 ||
             strcmp(tokens[0], "LIST_ROOMS") == 0 ||
             strcmp(tokens[0], "JOIN_ROOM") == 0 ||
             strcmp(tokens[0], "LEAVE_ROOM") == 0 ||
             strcmp(tokens[0], "QUIT") == 0) {
        send_message(socket_fd, "ERROR Invalid command or arguments\n");
    }
    else {
        log_event("UNKNOWN_COMMAND fd=%d command=%s", socket_fd, tokens[0]);
        snprintf(response, sizeof(response),
                 "ERROR Unknown command: %s\n", tokens[0]);
        send_message(socket_fd, response);
    }
}

void close_client(int index) {
    if (clients[index].socket_fd >= 0) {
        int user_index = clients[index].user_index;

        /* Neu mat ket noi, tu dong roi phong va dang xuat. */
        const char *username = (user_index >= 0 && user_index < user_count)
                                   ? users[user_index].username : "guest";
        int old_room_id = clients[index].current_room_id;
        log_event("CLIENT_DISCONNECTED fd=%d user=%s room_id=%d",
                  clients[index].socket_fd, username, old_room_id);
        remove_client_from_room(index);

        if (user_index >= 0 && user_index < user_count) {
            users[user_index].is_logged_in = 0;
            users[user_index].client_fd = -1;
        }

        close(clients[index].socket_fd);
        clients[index].socket_fd = -1;
        clients[index].length = 0;
        clients[index].user_index = -1;
        clients[index].current_room_id = -1;
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
        clients[i].current_room_id = -1;
    }

    memset(rooms, 0, sizeof(rooms));

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
    log_event("SERVER_START port=%d", PORT);

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
                        clients[i].current_room_id = -1;

                        send_message(client_fd,
                                     "OK Connected to auction server\n");
                        added = 1;
                        printf("Client connected: %d\n", client_fd);
                        log_event("CLIENT_CONNECTED fd=%d", client_fd);
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
                    char command_name[32] = {0};
                    size_t command_pos = 0;
                    while (clients[i].buffer[command_pos] != '\0' &&
                           !isspace((unsigned char)clients[i].buffer[command_pos]) &&
                           command_pos < sizeof(command_name) - 1) {
                        command_name[command_pos] = clients[i].buffer[command_pos];
                        command_pos++;
                    }
                    if (command_name[0] != '\0') {
                        log_event("COMMAND_RECEIVED fd=%d command=%s",
                                  client_fd, command_name);
                    }
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

    log_event("SERVER_STOP");
    close(server_fd);
    return 0;
}
