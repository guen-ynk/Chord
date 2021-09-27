//
// Created by lm on 25.11.19.
//

#ifndef BLOCK4_CLIENTSTORE_H
#define BLOCK4_CLIENTSTORE_H


#include "message.h"
#include "uthash.h"

typedef struct clientHashStruct_ {
    uint16_t key; // hashID von Key
    message* clientRequest; // request message
    int clientSocket; // socketID vom client der die message geschickt hat
    UT_hash_handle hh; /* makes this structure hashable */
} clientHashStruct;

clientHashStruct *createClientHash(uint16_t key, message* clientRequest, int clientSocket);
int setClientHash(uint16_t key, message* clientRequest, int clientSocket);
clientHashStruct *getClientHash(uint16_t key);
int deleteClientHash(uint16_t key);

#endif //BLOCK4_CLIENTSTORE_H
