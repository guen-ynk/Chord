#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "stdlib.h"

#include "../include/sockUtils.h"

/**
 * Erstellt einen neuen buffer mit der maximalen Länge "maxLength"
 *
 * @param maxLength
 * @return buffer
 */
buffer* createBuffer(uint32_t maxLength) {
    buffer *ret = malloc(sizeof(buffer));
    INVARIANT(ret != NULL, NULL, "Malloc error: buffer")

    ret->buff = malloc(maxLength);
    ret->maxLength = maxLength;
    ret->length = 0;

    return ret;
}

/**
 * Erstellt einen neuen Buffer ohne neuen Speicher zu allokieren.
 * Bekommt speicher als Argument "existingBuffer" übergeben.
 *
 * @param length
 * @param existingBuffer
 * @return buffer
 */
buffer* createBufferFrom(uint32_t length, void* existingBuffer) {
    buffer *ret = malloc(sizeof(buffer));
    INVARIANT(ret != NULL, NULL, "Malloc error: buffer")

    ret->buff = existingBuffer;
    ret->maxLength = length;
    ret->length = length;

    return ret;
}

buffer* copyBuffer(buffer* b) {
    uint8_t *cpy = calloc(b->length, sizeof(uint8_t));
    memcpy(cpy, b->buff, b->length);
    buffer* ret = createBufferFrom(b->length, cpy);
    return ret;
}

/**
 * Gibt den Speicher von einem buffer struct frei
 *
 * @param bufferToFree
 */
void freeBuffer(buffer *bufferToFree) {
    if(bufferToFree != NULL) {
        free(bufferToFree->buff);
        free(bufferToFree);
    }
}

/**
 * Recv "length" bytes über den angegebenen socket.
 *
 * @param socket
 * @param length
 * @return buffer mit den empfangenen Daten
 */
buffer* recvBytesAsBuffer(int socket, int length) {
    buffer *temp = createBuffer(length);
    INVARIANT(temp != NULL, NULL, "");

    while (temp->length < temp->maxLength) {
        int recvLength = recv(socket, (temp->buff + temp->length), temp->maxLength - temp->length, 0);
        INVARIANT_CB(recvLength != -1, NULL, "RecvError", {
            freeBuffer(temp);
        })

        temp->length += recvLength;
    }

    return temp;
}

/**
 * Recv "length" bytes über den angegebenen socket und speichere den vaue in buff
 *
 * @param socket
 * @param length
 * @param buff
 * @return anzahl an empfangen bytes, -1 bei Fehler
 */
int recvBytes(int socket, int length, void* buff) {
    int totalRecvLength = 0;

    while (totalRecvLength < length) {
        int recvLength = recv(socket, (buff + totalRecvLength), length - totalRecvLength, 0);
        INVARIANT(recvLength != -1, -1, "RecvError")

        totalRecvLength += recvLength;
    }

    INVARIANT(totalRecvLength == length, -1, "RecvError: Did not receive all bytes")

    return totalRecvLength;
}



/**
 * Hilfsfunktion die "length" bytes von value über den übergebenen socket verschickt.
 *
 * @param socket Socket über den "value" verschickt werden soll
 * @param value
 * @param length Anzahl an Bytes die von "value" verschickt werden sollen
 * @return -1 bei Fehler, ansonsten anzahl an verschickten bytes
 */
int sendAll(int socket, void* value, uint32_t length) {
    int totalSend = 0;

    // send until everything is send
    while (totalSend < length) {
        int sendn = send(socket, value + totalSend, length - totalSend, 0);
        INVARIANT(sendn != -1, -1, "Failed to send all bytes")

        totalSend += sendn;
    }

    INVARIANT(totalSend == length, -1, "Failed to send all bytes")

    return totalSend;
}

struct addrinfo getHints() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    return hints;
}

uint32_t addrinfoToServerAddress(struct addrinfo *addr) {
    struct sockaddr_in *serverIpAddr = (struct sockaddr_in*) addr->ai_addr;
    return serverIpAddr->sin_addr.s_addr;
}

int setupServer(char address[], char port[], uint32_t *ownIpAddr) {
    //own
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    //endown
    int status; // Keep track of possible errors

    // Get address info and store results in res
    status = getaddrinfo(address, port, &hints, &res);
    INVARIANT(status == 0, -1, "Failed to get address info")

    int socketServer;
    int yes = 1;
    for(p = res; p != NULL; p = p->ai_next) {
        // Create the socket using the address info in res
        socketServer = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        INVARIANT_CONTINUE_CB(socketServer != -1, "", {});

        // Set socket option to reuse the port.
        // Otherwise it fails after restarting the server, since the port is still in use.
        int sockOptStatus = setsockopt(socketServer, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        INVARIANT_CONTINUE_CB(sockOptStatus != -1, "", {
            close(socketServer);
        });

        // Bind the server address to the socket
        int bindStatus = bind(socketServer, p->ai_addr, p->ai_addrlen);
        INVARIANT_CONTINUE_CB(bindStatus != -1, "", {
            close(socketServer);
        });

        *ownIpAddr = addrinfoToServerAddress(p);

        break;
    }

    INVARIANT(p != NULL, -1, "Server: failed to bind");
    
    return socketServer;
}

int setupClient(char dnsAddress[], char port[]) {
    //own
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //endown
    int status; // Keep track of possible errors

    // Get address info and store results in res
    status = getaddrinfo(dnsAddress, port, &hints, &res);
    INVARIANT(status == 0, -1, "Failed to get address info")

    int clientSocket = 0;
    for (p = res; p != NULL; p = p->ai_next) {
        clientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        INVARIANT_CONTINUE_CB(clientSocket != -1, "", {})

        //Fehler Abfang falls Connection nicht established werden kann
        int connectStatus = connect(clientSocket, p->ai_addr, p->ai_addrlen);
        INVARIANT_CONTINUE_CB(connectStatus != -1, "", {
            close(clientSocket);
        })

        break;  //falls socket & connect geklappt haben -> haben wir eine passende Verbindung gefunden
    }

    //Fehler Abfang falls gar keine Adresse aus addressinfo gepasst hat
    INVARIANT(p != NULL, -1, "Client konnt nicht connecten");
    
    return clientSocket;
}

int setupClientWithAddr(uint32_t s_addr, uint16_t port) {
    //own
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //endown

    struct in_addr *addrIn = calloc(1, sizeof(struct in_addr));
    addrIn->s_addr = s_addr;

    struct sockaddr_in *addr = calloc(1, sizeof(struct sockaddr_in));
    addr->sin_addr = *addrIn;
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;

    int clientSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    INVARIANT(clientSocket != -1, -1, "Failed to create socket")

    int connectStatus = connect(clientSocket, (struct sockaddr *) addr, sizeof(struct sockaddr_in));
    INVARIANT_CB(connectStatus != -1, -1, "Failed to connect to lookup address",{
        close(clientSocket);
    })

    return clientSocket;
}

/**
 * Hilfsfunktion die schaut ob das n-te bit in der bitsequence gleich 1 ist.
 *
 * @param bitsequence
 * @param n
 * @return 1 wenn bit an stelle n 1 ist, ansonsten 0
 */
int checkBit(unsigned bitsequence, int n) {
    return (bitsequence >> n) & 1;
}
