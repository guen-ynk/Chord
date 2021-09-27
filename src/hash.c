#include "../include/hash.h"
#include "../include/sockUtils.h"
#include <stdio.h>

// HashTable init
hash_struct *store = NULL;

/**
 * Erstellt ein neues hash_struct struct welches im HashTable gespeichert werden kann
 *
 * @param key buffer
 * @param value buffer
 * @return hash_struct
 */
hash_struct *create(buffer *key, buffer *value) {
    hash_struct *newObj = malloc(sizeof(hash_struct));
    INVARIANT(newObj != NULL, NULL, "Failed to allocate memory for hash_struct")

    newObj->key = key;
    newObj->value = value;

    return newObj;
}

void freeHashStruct(hash_struct *h) {
    freeBuffer(h->value);
    freeBuffer(h->key);
    free(h);
}

/**
 * Schaut in HashTable nach ob es das Item mit dem Key "key" gibt
 *
 * @param key buffer Key von Item das gefunden werden soll
 * @return hash_struct wenn Item in HashTable ansonsten NULL
 */
hash_struct *get(buffer *key) {
    hash_struct *newItem;
    HASH_FIND(hh, store, key->buff, key->length, newItem);
    return newItem;
}

/**
 * Löscht das Item mit dem Key "key" aus der Datenbank.
 *
 * @param key buffer Key von Item das gelöscht werden soll
 * @return 0 wenn Item erfolgreich gelöscht wurde, ansonsten -1
 */
int delete(buffer *key) {
    hash_struct *tmp = get(key);
    if (tmp != NULL) {
        HASH_DELETE(hh, store, tmp);
        freeHashStruct(tmp);
    }

    hash_struct *proof = get(key);
    INVARIANT(proof == NULL, -1, "Did not delete item")

    return 0;
}

/**
 * Fügt neues Item dem HashTable hinzu
 *
 * @param key buffer
 * @param value buffer
 * @return -1 wenn Item schon vorhanden und nicht gelöscht werden kann, ansonsten 0
 */
int set(buffer *key, buffer *value) {
    hash_struct *newItem = create(key, value);

    // Delete if exists
    int deleteStatus = delete(key);
    INVARIANT(deleteStatus == 0, -1, "")

    HASH_ADD_KEYPTR(hh, store, key->buff, key->length, newItem);
    return 0;
}


