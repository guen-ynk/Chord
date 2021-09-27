#include "../include/sockUtils.h"
#include "../include/lookup.h"

#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>

lookup *createLookup(int reply, int isLookup, int stabilize, int notify, int join, int Finger, int FAck, uint16_t hashID, uint16_t nodeID, uint32_t nodeIP, uint16_t nodePort) {
    lookup *ret = calloc(1, sizeof(lookup));
    //own
    ret->join = join;
    ret->notify = notify;
    ret->stabilize = stabilize;
    ret->reply = reply;
    ret->lookup = isLookup;
    ret->Finger = Finger;
    ret->FAck = FAck;
    //endown
    ret->hashID = hashID;
    ret->nodeID = nodeID;
    ret->nodeIP = nodeIP;
    ret->nodePort = nodePort;
    return ret;
}

buffer *encodeLookup(lookup *l) {
    unsigned char *buff = calloc(11, sizeof(unsigned char));

    buff[0] = buff[0] | 0b10000000; // Control bit immer auf 1

    if (l->reply) buff[0] = buff[0] | 0b00000010;
    if (l->lookup) buff[0] = buff[0] | 0b00000001;
    //own
    if (l->stabilize) buff[0] = buff[0] | 0b00000100;
    if (l->notify) buff[0] = buff[0] | 0b00001000;
    if (l->join) buff[0] = buff[0] | 0b00010000;
    if (l->FAck==1) buff[0] = buff[0] | 0b00100000;
    if (l->Finger==1) buff[0] = buff[0] | 0b01000000;
    //endown
    uint16_t encodedNodeID = htons(l->nodeID);
    uint16_t encodedNodePort = htons(l->nodePort);

    memcpy(&buff[1], &l->hashID, 2); // bitstring
    memcpy(&buff[3], &encodedNodeID, 2);
    memcpy(&buff[5], &l->nodeIP, 4); // schon in network byte order
    memcpy(&buff[9], &encodedNodePort, 2);

    return createBufferFrom(11, buff);
}

/**
 * Helper der ein Packet zu einem Lookup Struct parsed.
 *
 * @param firstLine
 * @param buff
 * @return lookup
 */
lookup *decodeLookup(uint8_t firstLine, buffer* buff) {
    int reply = checkBit(firstLine, 1);
    int lookup = checkBit(firstLine, 0);
    //own
    int stabilze = checkBit(firstLine, 2);
    int notify = checkBit(firstLine, 3);
    int join = checkBit(firstLine, 4);
    int FAck = checkBit(firstLine, 5);
    int Finger = checkBit(firstLine, 6);
    //endown
    uint16_t hashID = 0;
    uint16_t nodeID = 0;
    uint32_t nodeIP = 0;
    uint16_t nodePort = 0;

    memcpy(&hashID, &buff->buff[0], 2);
    memcpy(&nodeID, &buff->buff[2], 2);
    memcpy(&nodeIP, &buff->buff[4], 4);
    memcpy(&nodePort, &buff->buff[8], 2);

    // nodeIP schon in network byte order
    // und da hashID ein bitstring ist, muss es nicht in eine bestimmte order
    return createLookup(reply, lookup, stabilze, notify, join, Finger, FAck, hashID, ntohs(nodeID), nodeIP, ntohs(nodePort));
}

int sendLookup(int socket, lookup* l) {
    buffer *b = encodeLookup(l);
    LOG_DEBUG("===========================\tSend Lookup\t=========================== ")
    printLookup(l);
    return sendAll(socket, b->buff, b->length);
}

lookup *recvLookup(int socket, uint8_t firstLine) {
    buffer* b = recvBytesAsBuffer(socket, 10);
    lookup* ret = decodeLookup(firstLine, b);
    LOG_DEBUG("===========================\tReceived Lookup\t=========================== ")
    printLookup(ret);

    return ret;
}

/**
 * Kopiert die ersten zwei bytes des keys in ein uint16_t int.
 * Falls weniger als zwei Bytes im Key vorhanden sind, werden nur keyLength bytes kopiert.
 *
 * @param key
 * @return uint_16t hash wert von key
 */
uint16_t getHashForKey(buffer *key) {
    uint16_t ret = 0;

    memcpy(&ret, key->buff, key->length > 2 ? 2 : key->length);

    return ret;
}
//own
uint16_t checkHashID(uint16_t hashID, serverArgs* args) {
// 1 Peer:
    if((args->ownID == args->nextID) && (args->nextID == args->prevID)){
        return OWN_SERVER;
    }
    // 2 Peers:
    if(args->nextID == args->prevID || args->ownID == args->nextID){ // oder wegen stabilize
        if((args->prevID<hashID) && (hashID<=args->ownID) && (args->ownID > args->prevID)){
            return OWN_SERVER;
        }
        if(args->ownID<args->prevID){
            if(args->prevID<hashID || hashID <= args->ownID){
                return OWN_SERVER;
            }
        }
        return NEXT_SERVER;
    }
    // Sonst 3 Peers
    // prev > own < next: prev ist vor 0 aber own und next nach 0 !
    if( args->ownID < args->prevID){
        if(args->prevID < hashID || args->ownID >= hashID){
            return OWN_SERVER;
        }
        if((args->nextID >= hashID) && (args->ownID < hashID)){
            return NEXT_SERVER;
        }
    }
    // prev < own > next: // prev und own sind vor 0 !
    if((args->prevID < args->ownID) && (args->ownID > args->nextID)){
        if((hashID > args->prevID) && (args->ownID >= hashID)){
            return OWN_SERVER;
        }
        if(args->ownID < hashID || hashID <= args->nextID){
            return  NEXT_SERVER;
        }
    }
    // prev < own < next: // easy
    if((args->prevID < args->ownID) && (args->ownID < args->nextID)){
        if((args->prevID < hashID) && (hashID <= args->ownID)){
            return OWN_SERVER;
        }
        if((args->ownID < hashID)&&(hashID <= args->nextID)){
            return NEXT_SERVER;
        }
    }
    // next < pre < own :
    if(args->nextID < args->ownID){
        if((args->prevID < hashID) && (hashID <= args->ownID)){
            return OWN_SERVER;
        }
        if(hashID <= args->nextID || args->ownID < hashID){
            return NEXT_SERVER;
        }
    }

    return UNKNOWN_SERVER;
}
//endown
void printLookup(lookup* l) {
#ifdef DEBUG
    if(l->stabilize == 0){
        fprintf(stderr, "{\n");
        fprintf(stderr, "\treply: %d\n", l->reply);
        fprintf(stderr, "\tlookup: %d\n", l->lookup);
        //own
        fprintf(stderr, "\tFinger: %d\n", l->Finger);
        fprintf(stderr, "\tFAck: %d\n", l->FAck);
        fprintf(stderr, "\tjoin: %d\n", l->join);
        fprintf(stderr, "\tnotify: %d\n", l->notify);
        fprintf(stderr, "\tstabilize: %d\n", l->stabilize);
        //endown
        fprintf(stderr, "\thashID: %d\n", l->hashID);
        fprintf(stderr, "\tnodeID: %d\n", l->nodeID);
        fprintf(stderr, "\tnodeIP: %d\n", l->nodeIP);
        fprintf(stderr, "\tnodePort: %d\n", l->nodePort);
        fprintf(stderr, "}\n");
    }
#endif
}