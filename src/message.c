#include "../include/message.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

/**
 * Erstellt ein neues message struct was verschickt werden kann.
 * Enthält die erste Zeile des Headers sowie den Key und den Value.
 *
 * @param instructions
 * @param key
 * @param value
 * @return message
 */
message *createMessage(int op, int ack, buffer *key, buffer *value) {
    message *m = calloc(1, sizeof(message));
    INVARIANT(m != NULL, NULL, "failed to allocate memory for message")

    m->op = op;
    m->ack = ack;
    m->key = key;
    m->value = value;

    return m;
}

/**
 * Gibt den Speicher für eine Message komplett frei.
 *
 * @param msg
 */
void freeMessage(message *msg) {
    freeBuffer(msg->key);
    freeBuffer(msg->value);
    free(msg);
}

message *copyMessage(message *msg) {
    buffer* keyCopy = msg->key && msg->key->length ? copyBuffer(msg->key) : msg->key;
    buffer* valueCopy = msg->value && msg->value->length ? copyBuffer(msg->value) : msg->value;

    return createMessage(msg->op, msg->ack, keyCopy, valueCopy);
}

/**
 * Hilfsfunktion die eine message über den angegebenen socket verschickt.
 * Wandelt die instructions in einen bitstring um und convertiert die
 * Key und Value Length in the Network Byte Order mit htons und htonl.
 *
 * @param socket
 * @param message
 * @return -1 bei Fehler, ansonsten 0
 */
int sendMessage(int socket, message *message) {

    // First Line
    uint8_t instrEncoded = 0;
    if (message->ack) instrEncoded = instrEncoded | 0b00001000;
    switch (message->op) {
        case GET_CODE:
            instrEncoded = instrEncoded | 0b00000100;
            break;
        case SET_CODE:
            instrEncoded = instrEncoded | 0b00000010;
            break;
        case DELETE_CODE:
            instrEncoded = instrEncoded | 0b00000001;
            break;
        case FINGERACK:
            instrEncoded = instrEncoded | 0b10100000;//own //endown
            break;
    }

    uint16_t keyDecoded = htons(message->key ? message->key->length : 0);
    uint32_t valueDecoded = htonl(message->value ? message->value->length : 0);
    int headerLengthSend = sendAll(socket, &instrEncoded, sizeof(uint8_t));
    INVARIANT(headerLengthSend == sizeof(uint8_t), -1, "Failed to send isntr");

    int keyLengthSend = sendAll(socket, &keyDecoded, sizeof(uint16_t));
    INVARIANT(keyLengthSend == sizeof(uint16_t), -1, "Failed to send keyLength");

    int valueLengthSend = sendAll(socket, &valueDecoded, sizeof(uint32_t));
    INVARIANT(valueLengthSend == sizeof(uint32_t), -1, "Failed to send valueLength");

    if (message->key != NULL && message->key->length != 0) {
        int keySend = sendAll(socket, message->key->buff, message->key->length);
        INVARIANT(keySend != -1, -1, "Failed to send key");
    }


    if (message->value != NULL && message->value->length != 0) {
        int valueSend = sendAll(socket, message->value->buff, message->value->length);
        INVARIANT(valueSend != -1, -1, "Failed to send value");
    }

    LOG_DEBUG("===========================\tSend Message\t=========================== ")
    printMessage(message);

    return 0;
}


/**
 * Hört auf dem übergebenen socket mit recv und empfängt eine neue message
 * im angegebenen Protokoll. Wandelt die Bitsequence in ein message struct
 * um und gibt es zurück.
 *
 * @param socket
 * @return message
 */
message *recvMessage(int socket, uint8_t firstLine) {
    if(firstLine != 138){
        int ack = checkBit(firstLine, 3);
        int get = checkBit(firstLine, 2);
        int set = checkBit(firstLine, 1);
        int finger = checkBit(firstLine, 6);
        int fingerack = checkBit(firstLine, 5);
        int delete = checkBit(firstLine, 0);

        int op = -1;
        if (get) op = GET_CODE;
        else if (set) op = SET_CODE;
        else if (delete) op = DELETE_CODE;
        else if (fingerack) op = FINGERACK;
        else if (finger) op = FINGER;

        INVARIANT(op != -1, NULL, "Invalid op code")

        uint16_t keyLength = 0;
        int receivedBytesKeyLength = recvBytes(socket, 2, &keyLength);
        INVARIANT(receivedBytesKeyLength != -1, NULL, "Failed to recv key length")
        keyLength = ntohs(keyLength);

        uint32_t valueLength = 0;
        int receivedBytesValueLength = recvBytes(socket, 4, &valueLength);
        INVARIANT(receivedBytesValueLength != -1, NULL, "Failed to recv value length")
        valueLength = ntohl(valueLength);

        buffer *keyBuffer = NULL;
        if (keyLength != 0) {
            keyBuffer = recvBytesAsBuffer(socket, keyLength);
            INVARIANT(keyBuffer != NULL, NULL, "Recv error: key");
        }

        buffer *valueBuffer = NULL;
        if (valueLength != 0) {
            valueBuffer = recvBytesAsBuffer(socket, valueLength);
            INVARIANT(valueBuffer != NULL, NULL, "Recv error: value");
        }


        message *msg = createMessage(op, ack, keyBuffer, valueBuffer);
        LOG_DEBUG("===========================\tReceived Message\t=========================== ")
        printMessage(msg);

        return msg;

    }else{
        buffer* b = recvBytesAsBuffer(socket, 10);
        return createMessage(4,0,NULL,NULL);
    }

}

void printMessage(message *message) {
#ifdef DEBUG
    fprintf(stderr, "{\n");
    fprintf(stderr, "\tACK: %d\n", message->ack);
    fprintf(stderr, "\tGET: %d\n", message->op == GET_CODE);
    fprintf(stderr, "\tSET: %d\n", message->op == SET_CODE);
    fprintf(stderr, "\tDELETE: %d\n", message->op == DELETE_CODE);
    //own
    fprintf(stderr, "\tFINGER: %d\n", message->op == FINGER);
    fprintf(stderr, "\tFINGERACK: %d\n", message->op == FINGERACK);
    //endown
    if (message->key && message->key->length) {
        fprintf(stderr, "\tKeyLength: %d\n", message->key->length);
        fprintf(stderr, "\tKey: ");
        fwrite((char *) message->key->buff, 1, message->key->length, stderr);
        fprintf(stderr, "\n");
    }

    if (message->value && message->value->length) {
        fprintf(stderr, "\tValueLength: %d\n", message->value->length);
        fprintf(stderr, "\tValue: ");
        fwrite((char *) message->value->buff, 1, message->value->length, stderr);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "}\n");
#endif
}

