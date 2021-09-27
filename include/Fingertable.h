//own
#ifndef BLOCK4_FINGERTABLE_H
#define BLOCK4_FINGERTABLE_H

#include "sockUtils.h"

typedef struct FingerTable {
    int size;
    struct Finger* curr;
    struct Finger* first;

} FingerTable;


typedef struct Finger {
    uint16_t  startID;
    uint16_t ID;
    uint32_t IP;
    uint16_t PORT;
    struct Finger *next_ptr; // elem next pointer
}Finger;

struct FingerTable * F_create(uint16_t id);
void F_destroy(struct FingerTable *q);
void printTable(struct FingerTable * q);



#endif //BLOCK5_FINGERTABLE_H
//endown