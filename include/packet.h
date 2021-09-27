#ifndef BLOCK4_PACKET_H
#define BLOCK4_PACKET_H

#include "message.h"
#include "lookup.h"

typedef struct _packet {
    int control;
    lookup *lookup;
    message *message;
} packet;

packet *createPacket(message* msg, lookup* l);
packet *recvPacket(int socket);
int freePacket(packet* p);

#endif //BLOCK4_PACKET_H
