//
// Created by lm on 25.11.19.
//

#include <stdio.h>
#include "../include/sockUtils.h"
#include "../include/clientStore.h"

clientHashStruct *clientRequestStore = NULL;

clientHashStruct *createClientHash(uint16_t key, message* clientRequest, int clientSocket) {
    clientHashStruct *ret = calloc(1, sizeof(clientHashStruct));

    ret->key = key;
    ret->clientRequest = clientRequest;
    ret->clientSocket = clientSocket;

    return ret;
}

void freeClientHash(clientHashStruct *c) {
    freeMessage(c->clientRequest);
    free(c);
}

int setClientHash(uint16_t key, message* clientRequest, int clientSocket) {
    clientHashStruct *item = getClientHash(key);

    if(item != NULL) {
        LOG("Same hashID already in clientRequestStore! ")
        return -1;
    }

    uint16_t *keyCpy = calloc(1, sizeof(uint16_t));
    memcpy(keyCpy, &key, 2);

    item = createClientHash(key, clientRequest, clientSocket);

    HASH_ADD_KEYPTR(hh, clientRequestStore, keyCpy, 2, item);

    return 0;
}

clientHashStruct *getClientHash(uint16_t key) {
    clientHashStruct *item;
    HASH_FIND(hh, clientRequestStore, &key, 2, item);

    return item;
}

int deleteClientHash(uint16_t key) {
    clientHashStruct *tmp = getClientHash(key);
    if (tmp != NULL) {
        HASH_DELETE(hh, clientRequestStore, tmp);
        freeClientHash(tmp);
    }

    clientHashStruct *proof = getClientHash(key);
    INVARIANT(proof == NULL, -1, "Did not delete item")

    return 0;
}


