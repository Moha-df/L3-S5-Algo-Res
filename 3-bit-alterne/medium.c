#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
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

#define BUFSIZE 2048

noreturn void usage(const char *msg)
{
    fprintf(stderr,
            "usage: %s port_local ip_émetteur port_émetteur ip_récepteur "
            "port_récepteur tx_perte\n",
            msg);
    exit(EXIT_FAILURE);
}

void perte(double error, int sockfd, char *buffer, ssize_t n)
{
    double r = (double)rand() / RAND_MAX;
    if (r > error) {
        CHK(send(sockfd, buffer, n, 0));
        printf("transmis\n");
    } else {
        printf("perte\n");
    }
    fflush(stdout);
}

void main_loop(double error, int sock_send, int sock_recv)
{
    /* deux sockets entrantes : E et R */
    struct pollfd pfds[2];
    pfds[0].fd = sock_send;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_recv;
    pfds[1].events = POLLIN;

    ssize_t n;
    char buffer[BUFSIZE];

    while (1) {
        /* attente de données sur l'un des deux sockets */
        CHK(poll(pfds, 2, -1));

        /* données de l'émetteur */
        if (pfds[0].revents & POLLIN) {
            CHK(n = recv(sock_send, buffer, BUFSIZE, 0));
            if (n > 0) {
                printf("E->R, seq=%d -> ", buffer[0]);
                perte(error, sock_recv, buffer, n);
            }
        }

        /* données du récepteur (contrôle) */
        if (pfds[1].revents & POLLIN) {
            CHK(n = recv(sock_recv, buffer, BUFSIZE, 0));
            printf("R->E, seq=%d -> ", buffer[0]);
            perte(error, sock_send, buffer, n);
        }
    }
}

int socket_factory(char *local_port, char *ip, char *port)
{
    int sockfd;

    /* structure d'adresse locale */
    if (local_port != NULL) {
        struct addrinfo *local, hints = {0};
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
        CHKA(getaddrinfo(NULL, local_port, &hints, &local));

        /* création d'un socket double pile IPv4 et IPv6 */
        int value = 0;
        CHK(sockfd = socket(local->ai_family, local->ai_socktype,
                            local->ai_protocol));
        CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value,
                       sizeof value));

        /* association socket et adresse/port */
        CHK(bind(sockfd, local->ai_addr, local->ai_addrlen));
        freeaddrinfo(local);
    }

    /* structure d'adresse distante */
    struct addrinfo *dest, hints2 = {0};
    hints2.ai_family = AF_UNSPEC;
    hints2.ai_socktype = SOCK_DGRAM;
    hints2.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    CHKA(getaddrinfo(ip, port, &hints2, &dest));

    if (local_port == NULL)
        CHK(sockfd =
                socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol));

    /* privatisation du socket */
    CHK(connect(sockfd, dest->ai_addr, dest->ai_addrlen));
    freeaddrinfo(dest);

    return sockfd;
}

void quit(int signo)
{
    (void)signo;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 7)
        usage(argv[0]);

    /* init générateur pseudo aléatoire */
    srand(time(NULL));

    /* quitte correctement le programme si SIGTERM */
    struct sigaction sa;
    sa.sa_handler = quit;
    sa.sa_flags = 0;
    CHK(sigemptyset(&sa.sa_mask));
    CHK(sigaction(SIGTERM, &sa, NULL));

    /* création des sockets */
    int sock_send, sock_recv;
    sock_send = socket_factory(argv[1], argv[2], argv[3]);
    sock_recv = socket_factory(NULL, argv[4], argv[5]);

    /* taux de pertes */
    double erreur;
    if (sscanf(argv[6], "%lf", &erreur) != 1 || erreur > 1.0 || erreur < 0) {
        fprintf(stderr, "usage : le taux d'erreur doit être compris dans l'intervalle [0,1]\n");
        exit(EXIT_FAILURE);
    }

    /* attente des messages */
    main_loop(erreur, sock_send, sock_recv);

    /* fermeture des sockets */
    CHK(close(sock_send));
    CHK(close(sock_recv));

    return 0;
}