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

int main(int argc, char *argv[])
{
    if (argc != 3)
        usage(argv[0]);
        
    const char *ip_addr = argv[1]; // Adresse IP de destination
    const char *port = argv[2];    // Port de destination

    //  Configuration de hints pour UDP
    struct addrinfo *res, *p, hints = {0};
    //struct addrinfo *res, hints = {0};

    hints.ai_family = AF_UNSPEC;    // IPv4 ou IPv6
    hints.ai_socktype = SOCK_DGRAM; // mode paquet
    hints.ai_protocol = IPPROTO_UDP; // Protocole UDP

    //  Obtenir les informations d'adresse de l'hôte
    CHKA(getaddrinfo(ip_addr, port, &hints, &res));

    //  Créer un socket UDP
    int sockfd = -1;
    while (p = res, p != NULL && sockfd == -1) {
        CHK((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)));
        p = p->ai_next;
    }
    //int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    //CHK(sockfd);

    //  Préparer le message et envoyer
    const char *message = "hello world";
    CHK(sendto(sockfd, message, strlen(message), 0, p->ai_addr, p->ai_addrlen));
    //CHK(sendto(sockfd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen));

    // Free la mémoire
    freeaddrinfo(res);

    return 0;
}
