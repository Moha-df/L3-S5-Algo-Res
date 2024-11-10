#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
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

// durée (en sec.) du temporisateur pour la retransmission (RTO)
#define TIMEOUT 2
#define BUFSIZE 1024 // taille max (en octets) pour les données dans un message
#define N 10          // modulo pour numéroter les messages
#define WNDSIZE 10


// format des messages
struct message {
    uint8_t seq;        // numéro de séquence du message
    char data[BUFSIZE]; // données contenues dans le message
};

noreturn void usage(const char *msg)
{
    fprintf(stderr, "usage: %s port_local ip_dest port_dest\n", msg);
    exit(EXIT_FAILURE);
}

/*
void send_loop(int socket)
{
    ssize_t n;
    struct message m;

    // on commence par envoyer le message 0 (cf. ligne 56)
    m.seq = 0;

    while ((n = read(STDIN_FILENO, m.data, BUFSIZE)) > 0) {
        // envoi des données
        m.seq = (m.seq + 1) % 2;
        CHK(send(socket, &m, 1 + n, 0));

        // attente (bloquante) ACK du récepteur
        ssize_t r;
        struct message tmp;

        // la condition à droite du OU est utile si la ligne est full-duplex,
        // ou en présence d'erreur sur contenu (cf. TD)
        while (((r = recv(socket, &tmp, sizeof tmp, 0)) == -1 && errno == EAGAIN) || tmp.seq != m.seq)
            // retransmission !
            CHK(send(socket, &m, 1 + n, 0));
        CHK(r);
    }
    CHK(n);
}
*/


sem_t window_space;           // Controls window size
pthread_mutex_t window_lock;   // Protects sequence numbers
int seq_start = 0, seq_end = 0;
struct message window[WNDSIZE];  // Window buffer for messages

void *send_thread(void *arg) {
    int socket = *(int *)arg;
    ssize_t n;

    while ((n = read(STDIN_FILENO, window[seq_end % WNDSIZE].data, BUFSIZE)) > 0) {
        sem_wait(&window_space);  // Block if window is full

        pthread_mutex_lock(&window_lock);
        window[seq_end % WNDSIZE].seq = seq_end % N;
        CHK(send(socket, &window[seq_end % WNDSIZE], 1 + n, 0));
        seq_end++;
        pthread_mutex_unlock(&window_lock);
    }
    return NULL;
}
void *ack_thread(void *arg) {
    int socket = *(int *)arg;
    struct message ack;
    ssize_t r;

    while ((r = recv(socket, &ack, sizeof(ack), 0)) > 0) {
        pthread_mutex_lock(&window_lock);
        int ack_seq = ack.seq;

        if (ack_seq == (seq_start % N)) {
            seq_start++;
            sem_post(&window_space);  // Free up space in the window
        }
        pthread_mutex_unlock(&window_lock);
    }
    return NULL;
}



int main(int argc, char *argv[])
{
    if (argc != 4)
        usage(argv[0]);

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

    // définition du temporisateur sur la non réception de données sur le socket
    struct timeval tv = {TIMEOUT, 0};
    CHK(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv));

    // paramètres du récepteur (medium ici)
    struct addrinfo *dest;
    hints.ai_family = AF_UNSPEC; // argv[2] pourra être une adresse IPv4 ou IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    CHKA(getaddrinfo(argv[2], argv[3], &hints, &dest));

    // privatise le socket pour une communication exclusive avec le medium
    // toutes les autres sources ou destinations seront filtrées par le SE
    CHK(connect(sockfd, dest->ai_addr, dest->ai_addrlen));
    freeaddrinfo(dest);

    // Initialize semaphore and mutex
    sem_init(&window_space, 0, WNDSIZE);
    pthread_mutex_init(&window_lock, NULL);

    pthread_t sender, receiver;
    pthread_create(&sender, NULL, send_thread, &sockfd);
    pthread_create(&receiver, NULL, ack_thread, &sockfd);

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    sem_destroy(&window_space);
    pthread_mutex_destroy(&window_lock);
    CHK(close(sockfd));

    return 0;
}