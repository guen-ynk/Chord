#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../include/packet.h"


packet *createPacket(message* msg, lookup* l) {
    INVARIANT(!(msg && l), NULL, "Can not create packet with message AND lookup")

    packet* ret = calloc(1, sizeof(packet));
    INVARIANT(ret != NULL, NULL, "Error calloc packet");

    if(msg) {
        ret->control = 0;
        ret->message = msg;
    } else if (l) {
        ret->control = 1;
        ret->lookup = l;
    }

    return ret;
}

packet *recvPacket(int socket) {
    uint8_t firstLine = 0;
    message* msg = NULL;
    lookup* l = NULL;



    int firstLineLength = recv(socket, &firstLine, 1, 0);
    INVARIANT(firstLineLength == 1, NULL, "Failed to recv first line of packet");


    if (checkBit(firstLine, 7)) {
        // Lookup Packet
        l = recvLookup(socket, firstLine);
    } else {
        // Message Packet ( GET, SET, DELETE )
        msg = recvMessage(socket, firstLine);
    }



    return createPacket(msg, l);
}

int freePacket(packet* p) {
    if(p->message) {
        freeMessage(p->message);
    }

    if(p->lookup) {
        free(p->lookup);
    }

    free(p);

    return 0;
}
