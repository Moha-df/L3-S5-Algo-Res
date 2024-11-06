#include <errno.h>
#include <netdb.h>
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

#define BUFSIZE 1024 // taille max (en octets) pour les données dans un message
#define N 2          // modulo pour numéroter les messages

// format des messages
struct message {
    uint8_t seq;        // numéro de séquence du message
    char data[BUFSIZE]; // données contenues dans le message
};

noreturn void usage(const char *msg)
{
    fprintf(stderr, "usage: %s port_local\n", msg);
    exit(EXIT_FAILURE);
}

void recv_loop(int socket)
{
    ssize_t n;
    uint8_t ma = 0; // message attendu
    struct message m;

    // émetteur peut utiliser une adresse IPv4 ou IPv6
    // struct sockaddr_storage est assez grande pour stocker tout type d'adresse
    struct sockaddr_storage ss;
    struct sockaddr *s = (struct sockaddr *)&ss;
    socklen_t addrlen = sizeof ss;

    while ((n = recvfrom(socket, &m, sizeof m, 0, s, &addrlen)) > 0) {
        if (m.seq == ma) { // on a reçu le message qu'on attendait
            CHK(write(STDOUT_FILENO, m.data, n - 1));
            ma = (ma + 1) % N;
        }

        // on envoie uniquement le num. de séquence du dernier message bien reçu
        CHK(sendto(socket, &m, 1, 0, s, addrlen));
    }
    CHK(n);
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

    // quitte correctement le programme si SIGTERM
    struct sigaction sa;
    sa.sa_handler = quit;
    sa.sa_flags = 0;
    CHK(sigemptyset(&sa.sa_mask));
    CHK(sigaction(SIGTERM, &sa, NULL));

    // création d'un socket IPv6 qui accepte les communications IPv4
    int sockfd, value = 0;
    CHK(sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP));
    CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof value));

    // structure d'adresse avec les paramètres locaux
    // (toutes les adresses IPv6 du poste + port passé en argument)
    // getaddrinfo fait ici une simple conversion
    // (pas de requêtes DNS ou de recherche dans les fichiers locaux)
    struct addrinfo *local, hints = {0};
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    CHKA(getaddrinfo(NULL, argv[1], &hints, &local));

    // association socket <-> adresse/port
    CHK(bind(sockfd, local->ai_addr, local->ai_addrlen));
    freeaddrinfo(local);

    // réception algo bit alterné
    recv_loop(sockfd);

    // fermeture socket
    CHK(close(sockfd));

    return 0;
}