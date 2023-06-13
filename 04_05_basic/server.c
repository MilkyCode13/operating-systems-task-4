#include "utils.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in garden_out_addr;

int in_connected = 0;
int out_connected = 0;

struct flowers flowers = {{0}};

void handle_garden_in_msg(struct message *msg) {
    if (msg->flower_num >= FLOWER_COUNT) {
        return;
    }

    flowers.flower[msg->flower_num] = 1;

    printf("Flower %zu has faded!\n", msg->flower_num);
}

void handle_garden_out_msg(struct sockaddr_in *addr) {
    garden_out_addr = *addr;
}

void handle_gardener_msg(int socket_fd, struct sockaddr_in *addr, struct message *msg) {
    if (sendto(socket_fd, &flowers.flower[msg->flower_num], sizeof(flowers.flower[msg->flower_num]), MSG_NOSIGNAL, (const struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0) {
        return;
    }

    if (flowers.flower[msg->flower_num]) {
        flowers.flower[msg->flower_num] = 0;

        if (sendto(socket_fd, msg, sizeof(struct message), MSG_NOSIGNAL, (const struct sockaddr *) &garden_out_addr, sizeof(garden_out_addr)) < 0) {
            return ;
        }

        printf("Flower %zu has been restored by gardener\n", msg->flower_num);
    }
}

int main(int argc, char **argv) {
    struct sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(11111);

    if (argc == 3) {
        if (inet_aton(argv[1], &socket_address.sin_addr) == 0) {
            fprintf(stderr, "Invalid address\n");
            return 1;
        }
        uint16_t port = strtoul(argv[2], NULL, 10);
        if (port == 0) {
            fprintf(stderr, "Invalid port");
            return 1;
        }
        socket_address.sin_port = htons(port);
    } else if (argc != 1) {
        return 1;
    }

    int socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        perror("socket");
        close(socket_fd);
        return 1;
    }

    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return 1;
    }

    if (bind(socket_fd, (const struct sockaddr *) &socket_address, sizeof(socket_address)) < 0) {
        perror("bind");
        close(socket_fd);
        return 1;
    }

    while (1) {
        struct message msg;
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        if (recvfrom(socket_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &addr, &addrlen) <= 0) {
            break;
        }

        switch (msg.client_type) {
            case GARDEN_IN_CLIENT:
                handle_garden_in_msg(&msg);
                break;
            case GARDEN_OUT_CLIENT:
                handle_garden_out_msg(&addr);
                break;
            case GARDENER_CLIENT:
                handle_gardener_msg(socket_fd, &addr, &msg);
                break;
        }
    }

    close(socket_fd);

    return 0;
}
