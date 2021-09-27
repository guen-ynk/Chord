//
// Created by lm on 30.11.19.
//

#include "../include/sockUtils.h"
#include "../include/peerClientStore.h"
#include <stdio.h>


peerToClientHashStruct *peerToClientStore = NULL;

peerToClientHashStruct *createPeerToClientHash(int peerSocket, int clientSocket) {
    peerToClientHashStruct *ret = calloc(1, sizeof(peerToClientHashStruct));

    ret->peerSocket = peerSocket;
    ret->clientSocket = clientSocket;

    return ret;
}

int setPeerToClientHash(int peerSocket, int clientSocket){
    peerToClientHashStruct *item = createPeerToClientHash(peerSocket, clientSocket);
    HASH_ADD_INT(peerToClientStore, peerSocket, item);
    return 0;
}

peerToClientHashStruct *getPeerToClientHash(int peerSocket) {
    peerToClientHashStruct *item;
    HASH_FIND_INT(peerToClientStore, &peerSocket, item);
    return item;
}

int deletePeerToClientHash(int peerSocket) {
    peerToClientHashStruct *tmp = getPeerToClientHash(peerSocket);
    if (tmp != NULL) {
        HASH_DELETE(hh, peerToClientStore, tmp);
        free(tmp);
    }

    peerToClientHashStruct *proof = getPeerToClientHash(peerSocket);
    INVARIANT(proof == NULL, -1, "Did not delete peerToClientHash")

    return 0;
}
