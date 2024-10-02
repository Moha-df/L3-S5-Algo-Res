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


noreturn void usage(const char *msg)
{
    fprintf(stderr, "usage: %s ip_dest port_dest\n", msg);
    exit(EXIT_FAILURE);
}


void copie(int src, int dst)
{
    char buf[1024];
    ssize_t n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t res = send(dst, buf + written, n - written, 0);
            if (res < 0) {
                perror("send");
                exit(EXIT_FAILURE);
            }
            written += res;
        }
    }
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        usage(argv[0]);
    
    const char *ip_dest = argv[1];
    const char *port_dest = argv[2];

    // Préparation de l'adresse distante
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC; // Accepte IPv4 et IPv6
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP
    hints.ai_flags = AI_NUMERICSERV;

    // Obtenir les informations d'adresse distante
    CHKA(getaddrinfo(ip_dest, port_dest, &hints, &res));

    // Création du socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    CHK(sockfd);

    // Établir une connexion avec l'adresse distante
    CHK(connect(sockfd, res->ai_addr, res->ai_addrlen));

    // Envoi des données
    copie(STDIN_FILENO, sockfd);

    // Fermer le socket
    close(sockfd);
    freeaddrinfo(res);

    return 0;
}
