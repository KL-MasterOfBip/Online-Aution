#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

static struct termios saved_terminal;
static int password_hidden = 0;
static volatile sig_atomic_t interrupted = 0;

/* Luon tra lai echo khi ket thuc, gap loi hoac nhan Ctrl+C. */
void restore_terminal(void) {
    if (password_hidden) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
        password_hidden = 0;
        putchar('\n');
        fflush(stdout);
    }
}

void handle_signal(int signum) {
    (void)signum;
    interrupted = 1;
}

int hide_password(void) {
    if (!isatty(STDIN_FILENO)) {
        return 0;
    }
    if (tcgetattr(STDIN_FILENO, &saved_terminal) < 0) {
        return -1;
    }
    struct termios terminal = saved_terminal;
    terminal.c_lflag &= (tcflag_t)~(ECHO | ECHONL);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal) < 0) {
        return -1;
    }
    password_hidden = 1;
    return 0;
}

int send_all(int socket_fd, const char *data, size_t length) {
    size_t total = 0;
    while (total < length && !interrupted) {
        ssize_t sent = send(socket_fd, data + total, length - total, 0);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent <= 0) {
            return -1;
        }
        total += (size_t)sent;
    }
    return total == length ? 0 : -1;
}

/* Phan hoi TCP theo dong; chi prompt password moi tat echo terminal. */
int display_response(char *line) {
    int password_prompt = strcmp(line, "New password:") == 0 ||
                          strcmp(line, "Password:") == 0;
    int username_prompt = strcmp(line, "New username:") == 0 ||
                          strcmp(line, "Username:") == 0;
    if (password_prompt && hide_password() < 0) {
        perror("Cannot hide password");
        return -1;
    }
    if (password_prompt || username_prompt) {
        printf("%s ", line);
    }
    else {
        puts(line);
    }
    fflush(stdout);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    long port = 8080;
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [IPv4 address] [port]\n", argv[0]);
        return 1;
    }
    if (argc > 2) {
        char *end;
        errno = 0;
        port = strtol(argv[2], &end, 10);
        if (errno || *end || end == argv[2] || port < 1 || port > 65535) {
            fprintf(stderr, "Invalid port\n");
            return 1;
        }
    }

    atexit(restore_terminal);
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    signal(SIGPIPE, SIG_IGN);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }
    if (socket_fd >= FD_SETSIZE) {
        fprintf(stderr, "Socket descriptor exceeds select limit\n");
        close(socket_fd);
        return 1;
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, host, &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address\n");
        close(socket_fd);
        return 1;
    }
    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect");
        close(socket_fd);
        return 1;
    }

    char response[BUFFER_SIZE];
    size_t response_length = 0;
    int stdin_open = 1;
    int result = 0;
    while (!interrupted) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        if (stdin_open) {
            FD_SET(STDIN_FILENO, &read_fds);
        }
        int ready = select(socket_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            result = 1;
            break;
        }
        if (FD_ISSET(socket_fd, &read_fds)) {
            char incoming[BUFFER_SIZE];
            ssize_t received = recv(socket_fd, incoming, sizeof(incoming), 0);
            if (received < 0 && errno == EINTR) {
                continue;
            }
            if (received <= 0) {
                if (received < 0) {
                    perror("recv");
                    result = 1;
                }
                break;
            }
            for (ssize_t i = 0; i < received; i++) {
                if (incoming[i] == '\n') {
                    response[response_length] = '\0';
                    if (display_response(response) < 0) {
                        result = 1;
                        goto done;
                    }
                    response_length = 0;
                }
                else if (response_length < sizeof(response) - 1) {
                    response[response_length++] = incoming[i];
                }
                else {
                    fprintf(stderr, "Server response too long\n");
                    result = 1;
                    goto done;
                }
            }
        }
        if (!interrupted && stdin_open && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[BUFFER_SIZE];
            ssize_t length = read(STDIN_FILENO, input, sizeof(input));
            if (length < 0 && errno == EINTR) {
                continue;
            }
            restore_terminal();
            if (length <= 0) {
                memset(input, 0, sizeof(input));
                if (length < 0) {
                    perror("read");
                    result = 1;
                    break;
                }
                shutdown(socket_fd, SHUT_WR);
                stdin_open = 0;
            }
            else {
                int sent = send_all(socket_fd, input, (size_t)length);
                memset(input, 0, sizeof(input));
                if (sent < 0) {
                    if (!interrupted) {
                        perror("send");
                    }
                    result = 1;
                    break;
                }
            }
        }
    }

done:
    restore_terminal();
    close(socket_fd);
    return result;
}
