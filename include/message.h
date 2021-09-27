#ifndef BLOCK4_MESSAGE_H
#define BLOCK4_MESSAGE_H

#include "../include/sockUtils.h"

#define GET_CODE 0
#define SET_CODE 1
#define DELETE_CODE 2
//own
#define FINGER 3
#define FINGERACK 4
//endown
#define ACKNOWLEDGED 1
#define NOT_ACKNOWLEDGED 0


typedef struct message_ {
    int op;
    int ack;
    buffer *key;
    buffer *value;
} message;

void freeMessage(message* msg);
message *copyMessage(message *msg);

message *createMessage(int op, int ack, buffer *key, buffer *value);

int sendMessage(int socket, message *message);
message *recvMessage(int socket, uint8_t firstLine);

void printMessage(message* message);

#endif //BLOCK4_MESSAGE_H
