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


noreturn void usage(const char *msg)
{
    fprintf(stderr,
            "usage: %s port_local ip_émetteur port_émetteur ip_récepteur "
            "port_récepteur tx_perte\n",
            msg);
    exit(EXIT_FAILURE);
}

void quit(int signo)
{
    (void)signo;
    exit(EXIT_SUCCESS);
}
//./medium port_local ip_emetteur port_emetteur ip_recepteur port_recepteur taux_perte
int main(int argc, char *argv[])
{
    if (argc != 7)
        usage(argv[0]);

    /* init générateur pseudo aléatoire */
    srand(time(NULL));

    const char *port_local = argv[1];
    const char *ip_emetteur = argv[2];
    const char *port_emetteur = argv[3];
    const char *ip_recepteur = argv[4];
    const char *port_recepteur = argv[5];
    int taux_perte = atoi(argv[6]);


//Pour send au recepteur .
    struct addrinfo hints_recep = {0}, *res_recep;
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = AI_NUMERICSERV | AI_NUMERICHOST;
    CHKA(getaddrinfo(ip_recepteur, port_recepteur, &hints_recep, &res_recep));
    int sockfd_recep = socket(res_recep->ai_family, res_recep->ai_socktype, res_recep->ai_protocol);
    CHK(sockfd_recep);
    CHK(connect(sockfd_recep, res_recep->ai_addr, res_recep->ai_addrlen));


// Pour recive de l'émetteur
    struct addrinfo hints_emet = {0}, *res_emet;
    hints.ai_family = AF_INET6; 
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = AI_NUMERICSERV | AI_NUMERICHOST;
    CHKA(getaddrinfo(ip_emetteur, port_emetteur, &hints_emet, &res_emet));
    int sockfd_emet = socket(res_emet->ai_family, res_emet->ai_socktype, res_emet->ai_protocol);
    CHK(sockfd_emet);
    CHK(connect(sockfd_emet, res_emet->ai_addr, res_emet->ai_addrlen));

    struct pollfds fds[2];
    
    fds[0].fd = sockfd_emet;   
    fds[0].events = POLLIN;

    fds[1].fd = sockfd_recep;
    fds[1].events = POLLOUT;    


    // Réception des données
    for (;;) {
        
    }

    



    return 0;
}
