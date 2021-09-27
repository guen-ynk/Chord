//
// Created by lm on 30.11.19.
//

#ifndef BLOCK4_PEERCLIENTSTORE_H
#define BLOCK4_PEERCLIENTSTORE_H

#include "message.h"
#include "uthash.h"

typedef struct peerToClientHashStruct_ {
    int peerSocket; // peerSocket
    int clientSocket;
    UT_hash_handle hh; /* makes this structure hashable */
} peerToClientHashStruct;

peerToClientHashStruct *createPeerToClientHash(int peerSocket, int clientSocket);
int setPeerToClientHash(int peerSocket, int clientSocket);
peerToClientHashStruct *getPeerToClientHash(int peerSocket);
int deletePeerToClientHash(int peerSocket);

#endif //BLOCK4_PEERCLIENTSTORE_H
