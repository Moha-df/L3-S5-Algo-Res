#include <netdb.h>
#include <signal.h>
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
    fprintf(stderr, "usage: %s port_local\n", msg);
    exit(EXIT_FAILURE);
}

void copie(int src, int dst)
{
    char buf[1024];
    ssize_t n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        write(dst, buf, n);
    }
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

}

void quit(int signo)
{
    (void)signo;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv[0]);
    const char *port = argv[1];

    // Préparation de l'adresse locale
    struct addrinfo hints ={0}, *res;
    hints.ai_family = AF_INET6;      // Accepte IPv4 et IPv6
    hints.ai_socktype = SOCK_DGRAM;   // Socket UDP
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;      // Ecouter sur toutes les interfaces

    // Obtenir les informations d'adresse locale
    CHKA(getaddrinfo(NULL, port, &hints, &res));

    // Création du socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    CHK(sockfd);

    // Configuration du socket pour accepter IPv4 et IPv6
    int value = 0;
    CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof(value)));

    // Association du socket à l'adresse et au port local
    CHK(bind(sockfd, res->ai_addr, res->ai_addrlen));

    // Gestion du signal SIGTERM
    struct sigaction sa;
    sa.sa_handler = quit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    CHK(sigaction(SIGTERM, &sa, NULL));

    // Réception des données
    for (;;) {
        copie(sockfd, STDOUT_FILENO);
    }

    // Libérer la mémoire de l'adresse
    freeaddrinfo(res);
    close(sockfd);

    return 0;
}
