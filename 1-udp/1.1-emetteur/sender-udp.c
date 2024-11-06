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
    
    /*
        Passage des arguments dans des variables
    */
    const char *ip_addr = argv[1];
    const char *port = argv[2];

    /*
        On cree 3 structures
        hints initialiser a 0 cest celle quon utilisera pour donner les directives
        res pour stocker le resultat de getaddrinfo
        p pour parcourir res a la creation du socket
    */
    struct addrinfo hints = {0};
    struct addrinfo* res = NULL;
    struct addrinfo* p = NULL;

    /*
        On donne les attribu a hints 
        SOCK_DGRAM IPPROTO_UDP = udp
        AI_NUMERICSERV = port numeric
        AF_UNSPEC = IPV6 ou IPV4 on specifie pas car specifier pourrait faire un prblm idk why
    */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags =  AI_NUMERICSERV;

    /*
        Lancement de getaddrinfo qui va remplir la structure res
    */
    CHKA(getaddrinfo(ip_addr, port, &hints, &res));

    /*
        On parcour res pour cree un socket valide
    */
    int sockfd = -1;
    while (p = res, p != NULL && sockfd == -1) {
        CHK((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)));
        p = p->ai_next;
    }

    /*
        On send le message
    */
    const char *message = "hello world";
    CHK(sendto(sockfd, message, strlen(message), res->ai_flags, res->ai_addr, res->ai_addrlen));
    
    /*
        Free
    */
    freeaddrinfo(res);

    return 0;
}
