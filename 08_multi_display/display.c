#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

int socket_fd;
struct sockaddr_in server_address;

void cleanup() {
    close(socket_fd);
}

void run_display() {
    while (1) {
        char buffer[128];
        ssize_t bytes = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }

        printf("%s", buffer);
    }
    close(socket_fd);
}

int main(int argc, char **argv) {
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(11111);

    if (argc == 3) {
        if (inet_aton(argv[1], &server_address.sin_addr) == 0) {
            fprintf(stderr, "Invalid address\n");
            return 1;
        }
        uint16_t port = strtoul(argv[2], NULL, 10);
        if (port == 0) {
            fprintf(stderr, "Invalid port");
            return 1;
        }
        server_address.sin_port = htons(port);
    } else if (argc != 1) {
        return 1;
    }

    socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        perror("socket");
        close(socket_fd);
        return 1;
    }

    signal(SIGINT, cleanup);

    struct message msg;
    msg.client_type = DISPLAY_CLIENT;
    if (sendto(socket_fd, &msg, sizeof(msg), MSG_NOSIGNAL, (const struct sockaddr *) &server_address, sizeof(server_address)) <= 0) {
        close(socket_fd);
        return 1;
    }

    run_display();

    return 0;
}
