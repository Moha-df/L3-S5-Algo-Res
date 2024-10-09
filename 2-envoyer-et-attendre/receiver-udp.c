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

#define BUFFERSIZE 1024  // Taille fixe des blocs de réception

noreturn void usage(const char *msg) {
    fprintf(stderr, "usage: %s port_local\n", msg);
    exit(EXIT_FAILURE);
}

void copie(int sockfd) {
    char buf[BUFFERSIZE];
    ssize_t n;
    struct sockaddr_storage sender_addr; 
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (1) {
        n = recvfrom(sockfd, buf, BUFFERSIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (n < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        // Écriture des données reçues
        write(STDOUT_FILENO, buf, n);

        // Envoie un ACK
        const char *ack = "ACK";
        sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&sender_addr, sender_addr_len);
    }
}

void quit(int signo) {
    (void)signo;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        usage(argv[0]);

    const char *port = argv[1];

    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET6;     
    hints.ai_socktype = SOCK_DGRAM;  
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;  

    CHKA(getaddrinfo(NULL, port, &hints, &res));

    // Création du socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    CHK(sockfd);

    // Bind du socket
    CHK(bind(sockfd, res->ai_addr, res->ai_addrlen));

    // Gestion du signal SIGTERM
    struct sigaction sa;
    sa.sa_handler = quit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    CHK(sigaction(SIGTERM, &sa, NULL));

    freeaddrinfo(res);

    // Réception des données
    for (;;) {
        copie(sockfd);
    }

    close(sockfd);

    return 0;
}
