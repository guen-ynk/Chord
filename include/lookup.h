#ifndef BLOCK3_LOOKUP_H
#define BLOCK3_LOOKUP_H

#include "sockUtils.h"

typedef struct _lookup {
    int reply;
    int lookup;
    //own
    int stabilize;
    int notify;
    int join;
    int Finger;
    int FAck;
    //endown
    uint16_t hashID;
    uint16_t nodeID;
    uint32_t nodeIP;
    uint16_t nodePort;
} lookup;


buffer *encodeLookup(lookup *l);

lookup *createLookup(int reply, int isLookup,int stabilize, int notify, int join, int Finger, int FAck, uint16_t hashID, uint16_t nodeID, uint32_t nodeIP, uint16_t nodePort);
lookup *decodeLookup(uint8_t firstLine, buffer* buff);

int sendLookup(int socket, lookup* l);
lookup *recvLookup(int socket, uint8_t firstLine);

uint16_t getHashForKey(buffer *key);
uint16_t checkHashID(uint16_t hashID, serverArgs* args);

void printLookup(lookup* l);

#endif //BLOCK3_LOOKUP_H
