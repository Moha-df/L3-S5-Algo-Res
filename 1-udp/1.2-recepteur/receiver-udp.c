#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SIZE 1024

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
    fprintf(stderr, "usage: %s port_dest\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv[0]);
    const char *port = argv[1];  // Port de destination

    // 1. Préparation de l'adresse locale
    struct addrinfo hints ={0}, *res;
    hints.ai_family = AF_INET6;     // IPv4 ou IPv6
    hints.ai_socktype = SOCK_DGRAM;  // Socket UDP
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;     // Ecouter sur toutes les interfaces

    // Obtenir les informations d'adresse locale
    CHKA(getaddrinfo(NULL, port, &hints, &res));

    // 2. Création du socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    CHK(sockfd);

// 3. Configuration du socket pour accepter IPv4 et IPv6
    int value = 0;
    CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof(value)));


    // 4. Association du socket à l'adresse et au port local
    CHK(bind(sockfd, res->ai_addr, res->ai_addrlen));

    // 5.6. Conversion des informations de l'expéditeur
    struct sockaddr_storage ss;
    struct sockaddr *s = (struct sockaddr *)&ss;
    socklen_t addrlen = sizeof ss;
    ssize_t n;
    char buf[SIZE] = {0};
    CHK(n = recvfrom(sockfd, buf, SIZE - 1, 0, s, &addrlen));
    buf[n] = '\0';  // Nul-termine le message reçu




    // Appel à getnameinfo pour résoudre l'adresse en nom d'hôte et port
    char host[NI_MAXHOST];
    char portt[NI_MAXSERV];
    int status = getnameinfo(s, addrlen, host, NI_MAXHOST, portt, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if (status != 0) {
        fprintf(stderr, "getnameinfo : %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }



    // affichage ->
    printf("%s %s a envoyé : %s", host, portt, buf);



    // Fermer le socket
    close(sockfd);
    
    // Libérer la mémoire de l'adresse
    freeaddrinfo(res);

    return 0;
}
