#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CHK(op)                                                                \
    do {                                                                       \
        if ((op) == -1) {                                                      \
            perror(#op);                                                       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define CHKA(op)                                                               \
    do {                                                                       \
        int error = 0;                                                         \
        if ((error = (op)) != 0) {                                             \
            fprintf(stderr, #op " %s\n", gai_strerror(error));                 \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

#define BUFFERSIZE 1024  // Définir la taille du tampon de réception

noreturn void usage(const char *msg) {
    fprintf(stderr, "usage: %s ip_dest port_dest\n", msg);
    exit(EXIT_FAILURE);
}

void copie(int src, int sockfd, const struct sockaddr *dest_addr, socklen_t addr_len) {
    char buf[BUFFERSIZE];
    ssize_t n;

    while ((n = read(src, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;

        // Envoi des données par blocs
        while (written < n) {
            ssize_t res = sendto(sockfd, buf + written, n - written, 0, dest_addr, addr_len);
            if (res < 0) {
                perror("sendto");
                exit(EXIT_FAILURE);
            }
            written += res;
        }

        // Attente de l'ACK
        char ack[4]; // Buffer pour l'ACK
        ssize_t ack_len = recv(sockfd, ack, sizeof(ack), 0);
        if (ack_len < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        // Afficher l'ACK
        if (ack_len > 0) {
            ack[ack_len] = '\0'; // Terminer l'ACK avec un caractère nul
            write(STDOUT_FILENO, ack, ack_len);
        }
    }

    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        usage(argv[0]);

    const char *ip_dest = argv[1];
    const char *port_dest = argv[2];

    // Préparation de l'adresse distante
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = AI_NUMERICSERV;

    CHKA(getaddrinfo(ip_dest, port_dest, &hints, &res));

    // Création du socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    CHK(sockfd);

    // Envoi des données
    copie(STDIN_FILENO, sockfd, res->ai_addr, res->ai_addrlen);

    close(sockfd);
    freeaddrinfo(res);

    return 0;
}
