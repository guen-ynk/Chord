#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "./include/hash.h"
#include "./include/lookup.h"
#include "./include/packet.h"
#include "./include/clientStore.h"
#include "./include/peerClientStore.h"
#include "./include/Fingertable.h"
//own
int ARGC;
int joined;
int firstsolo;
int lookupmode;

struct timeval tv;          // Select Timeout struct aka counter
struct timeval t0;          // Select Timeout struct aka counter
struct timeval t1;          // Select Timeout struct aka counter
struct FingerTable FTable;

int getpeer(int id, serverArgs *args){
    if(lookupmode == 0){
        return setupClientWithAddr(args->nextIpAddr, atoi(args->nextPort));
    }
    serverArgs *ret = calloc(1, sizeof(serverArgs));
    ret->ownID = args->ownID;
    ret->nextID =  args->nextID;
    ret->prevID = args->prevID;
    int caseMode = checkHashID(id, ret);
    FTable.curr = FTable.first;
    if(caseMode == OWN_SERVER){
        return -1;
    }
    while(FTable.curr != NULL ){
        ret->prevID = ret->ownID;
        ret->ownID = FTable.curr->ID;
        ret->nextID = FTable.curr->next_ptr->ID;
        caseMode = checkHashID(id, ret);
        if(caseMode != UNKNOWN_SERVER){
            break;
        }
        FTable.curr = FTable.curr->next_ptr;
    }
    if(caseMode == OWN_SERVER){
        fprintf(stderr, "-----------------FINGERCHECKDEBUG-----------------\n ID: %d \n FOUND ID: %d \n caseMode: %d \n", id,FTable.curr->ID,caseMode);
        return setupClientWithAddr(FTable.curr->IP, FTable.curr->PORT);

    }
    else{
        fprintf(stderr, "-----------------FINGERCHECKDEBUG-----------------\n ID: %d \n FOUND ID: %d \n caseMode: %d \n", id,FTable.curr->next_ptr->ID,caseMode);
        return setupClientWithAddr(FTable.curr->next_ptr->IP, FTable.curr->next_ptr->PORT);
    }

        // case unknown !!!
}
//endown
int handleHashTableRequest(int socket, message *msg) {
    message *responseMessage;
    switch (msg->op) {
        case DELETE_CODE: {
            int deleteStatus = delete(msg->key);
            INVARIANT(deleteStatus == 0, -1, "Failed to delete")
            responseMessage = createMessage(DELETE_CODE, ACKNOWLEDGED, NULL, NULL);
            break;
        }
        case GET_CODE: {
            hash_struct *obj = get(msg->key);
            responseMessage = createMessage(GET_CODE, ACKNOWLEDGED, copyBuffer(msg->key),
                                            obj != NULL ? copyBuffer(obj->value) : NULL);
            break;
        }
        case SET_CODE: {
            int setStatus = set(copyBuffer(msg->key), copyBuffer(msg->value));
            INVARIANT(setStatus == 0, -1, "Failed to set");
            responseMessage = createMessage(SET_CODE, ACKNOWLEDGED, NULL, NULL);
            break;
        }
        default: {
            LOG("Invalid instructions");
        }
    }

    INVARIANT(responseMessage != NULL, -1, "");
    int status = sendMessage(socket, responseMessage);
    freeMessage(responseMessage);
    INVARIANT(status == 0, -1, "Failed to send message");

    return 0;
}
//own
int check(serverArgs *args, packet *pkt){
  
    if(firstsolo == 1 || args->prevID==args->ownID){// wenn startknoten oder kein vorgänger gesetzt
        return 1;
    }

    //zwei oder mehr knoten
    if(args->prevID>args->ownID){//   ... prev  0  self ...
        if(pkt->lookup->nodeID > args->prevID || pkt->lookup->nodeID < args->ownID){
            return 1;
        }
    } else if(pkt->lookup->nodeID > args->prevID && pkt->lookup->nodeID < args->ownID){// 0 ... prev self ...  0
        return 1;
    }
    return 0;
}
//endown
int handlePacket(packet *pkt, int sock, fd_set *master, serverArgs *args, int *fdMax) {
    if (pkt->control) {
        fprintf(stderr, "FTABLE.size: %d", FTable.size);
        /******* Handle Lookup/Reply Request  *******/
        if (pkt->lookup->lookup) {
            int hashDestination = checkHashID(pkt->lookup->hashID, args);

            switch (hashDestination) {
                // Case 1 Der Key liegt auf dem next Server -> REPLY an lookup server schicken mit next IP, next PORT, next ID
                case NEXT_SERVER: {
                    lookup *responseLookup = createLookup(1, 0,0,0,0,0,0, pkt->lookup->hashID, args->nextID, args->nextIpAddr,
                                                          atoi(args->nextPort));

                    int peerSock = setupClientWithAddr(pkt->lookup->nodeIP, pkt->lookup->nodePort);
                    sendLookup(peerSock, responseLookup);
                    free(responseLookup);
                    close(peerSock);
                    break;
                }

                    // Case 2 Der key liegt auf einem unbekannten Server -> Lookup an nächsten Server
                case UNKNOWN_SERVER: {
                    int peerSock = getpeer(pkt->lookup->hashID, args);
                    sendLookup(peerSock, pkt->lookup);
                    close(peerSock);
                }
            }
            // Sock kann bei einer Control Nachricht immer geschlossen werden
            FD_CLR(sock, master);
            close(sock);

        }
        else if (pkt->lookup->reply) {
            // Case 2 REPLY  -> GET/SET/DELETE Anfrage an Server schicken und das Ergebnis an Client schicken
            clientHashStruct *s = getClientHash(pkt->lookup->hashID);
         //  fprintf(stderr,"\n\n\n\n\n\n\ntestw %d\n\n\n\n\n\n",s->clientRequest->op);
            if(s != NULL ){
                if(s->clientRequest->op != 4){
                    int peerSock = setupClientWithAddr(pkt->lookup->nodeIP, pkt->lookup->nodePort);
                    FD_SET(peerSock, master);
                    if (*(fdMax) < peerSock) {
                        *fdMax = peerSock;
                    }
                    setPeerToClientHash(peerSock, s->clientSocket);
                    int sendMessageStatus = sendMessage(peerSock, s->clientRequest);
                    INVARIANT_CB(sendMessageStatus != -1, -1, "Failed to send Message", {
                        close(peerSock);
                    });
                    deleteClientHash(pkt->lookup->hashID);
                }
            } else{
                //own
                while( NULL != FTable.curr ){
                    if(FTable.curr->startID == pkt->lookup->hashID){
                        FTable.curr->ID = pkt->lookup->nodeID;
                        FTable.curr->IP = pkt->lookup->nodeIP;
                        FTable.curr->PORT = pkt->lookup->nodePort;
                        FTable.size --;
                        break;
                    }
                    FTable.curr = FTable.curr->next_ptr;
                }
                FTable.curr = FTable.first;

                if(FTable.size == 0){
                    clientHashStruct *s = getClientHash(args->ownID);
                    lookup * f = createLookup(0,0,0,0,0,0,1,0,0,0,0);
                    fprintf(stderr, "-----------------SEND FACK------------------");
                    printLookup(f);
                    sendLookup(s->clientSocket,f);
                    close(s->clientSocket);
                    FD_CLR(s->clientSocket, master);
                    deleteClientHash(args->ownID);
                    lookupmode = 1;
                    printTable(&FTable);

                }
            }
            // Sock kann bei einer Control Nachricht immer geschlossen werden
            FD_CLR(sock, master);
            close(sock);
            return 1;
        }
        else if (pkt->lookup->join) {
            // CASE 3 JOIN REQUEST VON NEUEM PEER
            int hashDestination = checkHashID(pkt->lookup->hashID, args);
            // IF 0 -> NOTIFY ELSE:  WEITER
            if(hashDestination == OWN_SERVER){
                // Aktualiert Pred:
                int checkval = check(args, pkt);

                    args->prevID = pkt->lookup->nodeID;
                    args->prevPort = calloc(1,sizeof(pkt->lookup->nodePort));
                    sprintf(args->prevPort, "%d", pkt->lookup->nodePort);
                    args->prevIpAddr = pkt->lookup->nodeIP;

                    // WENN NEXT->PORT == NULL , JOIN AUF ERSTEN NODE UND DAMIT IST DER ERSTE NEULING PREV UND NEXT ZUGLEICH !!!
                    if(firstsolo == 1){
                        args->nextID = pkt->lookup->nodeID;
                        args->nextPort = calloc(1,sizeof(pkt->lookup->nodePort));
                        sprintf(args->nextPort, "%d", pkt->lookup->nodePort);
                        args->nextIpAddr = pkt->lookup->nodeIP;

                        FTable.first->PORT = (atoi(args->nextPort));
                        FTable.first->IP = args->nextIpAddr;
                        FTable.first->ID = args->nextID;
                        firstsolo = 0;
                        joined = 1;
                        gettimeofday(&t0, 0);
                    }
                // NOTIFY
                int peerSock = setupClientWithAddr( pkt->lookup->nodeIP, pkt->lookup->nodePort);
                lookup * notify = createLookup(0, 0, 0, 1, 0,0,0, args->ownID, args->ownID, args->ownIpAddr, atoi(args->ownPort));
                buffer *b  = encodeLookup(notify);
                sendAll(peerSock, b->buff, b->length);
                free(b);
                close(peerSock);

            }else{
                int peerSock = getpeer(pkt->lookup->hashID, args);
                sendLookup(peerSock, pkt->lookup);
                close(peerSock);
            }
            // Sock kann bei einer Control Nachricht immer geschlossen werden
            FD_CLR(sock, master);
            close(sock);
        }
        else if(pkt->lookup->notify){
            // Nachfolger aktualisieren !!!!!!!!
            if(joined == 1){
                if(args->ownID != pkt->lookup->nodeID) {
                    args->nextID = pkt->lookup->nodeID;
                    args->nextPort = calloc(1, sizeof(pkt->lookup->nodePort));
                    sprintf(args->nextPort, "%d", pkt->lookup->nodePort);
                    args->nextIpAddr = pkt->lookup->nodeIP;
                    FTable.first->PORT = (atoi(args->nextPort));
                    FTable.first->IP = args->nextIpAddr;
                    FTable.first->ID = args->nextID;
                }
            }
            else{
                args->nextID = pkt->lookup->nodeID;
                args->nextPort = calloc(1,sizeof(pkt->lookup->nodePort));
                sprintf(args->nextPort, "%d", pkt->lookup->nodePort);
                args->nextIpAddr = pkt->lookup->nodeIP;
                FTable.first->PORT = (atoi(args->nextPort));
                FTable.first->IP = args->nextIpAddr;
                FTable.first->ID = args->nextID;
                joined = 1;
                gettimeofday(&t0, 0);
            }
            // Sock kann bei einer Control Nachricht immer geschlossen werden
            FD_CLR(sock, master);
            close(sock);
            return 1;
        }
        else if(pkt->lookup->stabilize){
            int checkval = check(args, pkt);
            if(checkval == 1){
                args->prevID = pkt->lookup->nodeID;
                args->prevPort = calloc(1,sizeof(pkt->lookup->nodePort));
                sprintf(args->prevPort, "%d", pkt->lookup->nodePort);
                args->prevIpAddr = pkt->lookup->nodeIP;
            }
            int peerSock = setupClientWithAddr(pkt->lookup->nodeIP, pkt->lookup->nodePort);
            lookup * notify = createLookup(0, 0, 0, 1, 0,0,0, args->ownID, args->prevID, args->prevIpAddr, atoi(args->prevPort));
            buffer*b = encodeLookup(notify);
            sendAll(peerSock, b->buff, b->length);
            close(peerSock);
            free(notify);
            free(b);
            // Sock kann bei einer Control Nachricht immer geschlossen werden
            FD_CLR(sock, master);
            close(sock);
            return 1;
        }
        else if(pkt->lookup->Finger) {
            if (lookupmode == 0) {
                // nun m-1 freie Einräge die es zu füllen gilt
                // aufruf für ersten Spot starten -> lookup : head ,  Gesuchte ID ,ID , IP, PORT:::: bis n-1
                message *m = createMessage(4, 0, NULL, NULL);
                int status = setClientHash(args->ownID, m, sock);
                // keine  weiteren Daten werden gespeichert
                if (status == -1) {
                    close(sock);
                    return -1;
                }
                FTable.size = 15;
                FTable.curr = FTable.first->next_ptr;
                while (NULL != FTable.curr) {
                    int checkh = checkHashID(FTable.curr->startID, args);
                    if (checkh == OWN_SERVER) {
                        FTable.curr->ID = args->ownID;
                        FTable.curr->IP = args->ownIpAddr;
                        FTable.curr->PORT = atoi(args->ownPort);
                        FTable.size--;
                        if (FTable.size == 0) {
                            clientHashStruct *s = getClientHash(args->ownID);
                            lookup *f = createLookup(0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
                            fprintf(stderr, "-----------------SEND FACK------------------");
                            printLookup(f);
                            sendLookup(s->clientSocket, f);
                            close(s->clientSocket);
                            FD_CLR(s->clientSocket, master);
                            deleteClientHash(args->ownID);
                            lookupmode = 1;
                            printTable(&FTable);

                        }
                    } else if (checkh == NEXT_SERVER) {
                        FTable.curr->ID = args->nextID;
                        FTable.curr->IP = args->nextIpAddr;
                        FTable.curr->PORT = atoi(args->nextPort);
                        FTable.size--;
                        if (FTable.size == 0) {
                            clientHashStruct *s = getClientHash(args->ownID);
                            lookup *f = createLookup(0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
                            fprintf(stderr, "-----------------SEND FACK------------------");
                            printLookup(f);
                            sendLookup(s->clientSocket, f);
                            close(s->clientSocket);
                            FD_CLR(s->clientSocket, master);
                            deleteClientHash(args->ownID);
                            lookupmode = 1;
                            printTable(&FTable);

                        }
                    } else {
                        int peerSock = getpeer(FTable.curr->startID, args);
                        lookup *finger = createLookup(0, 1, 0, 0, 0, 0, 0, FTable.curr->startID, args->ownID,
                                                      args->ownIpAddr, atoi(args->ownPort));
                        printLookup(finger);
                        buffer *fingers = encodeLookup(finger);
                        sendAll(peerSock, fingers->buff, fingers->length);
                        close(peerSock);
                        free(finger);
                        free(fingers);
                    }
                    FTable.curr = FTable.curr->next_ptr;
                }
                FTable.curr = FTable.first;
                FD_CLR(sock, master);
            }

            else{
                lookup *f = createLookup(0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
                fprintf(stderr, "-----------------SEND FACK------------------");
                printLookup(f);
                sendLookup(sock, f);
                close(sock);
                FD_CLR(sock, master);
                lookupmode = 1;
                printTable(&FTable);
            }

        }
        //endown
    }
    else {
        /******* Handle GET/SET/DELETE Request  *******/
        if (pkt->message->ack) {

            peerToClientHashStruct *pHash = getPeerToClientHash(sock);
            close(pHash->peerSocket);
            FD_CLR(pHash->peerSocket, master);


            sendMessage(pHash->clientSocket, pkt->message);
            close(pHash->clientSocket);
            FD_CLR(pHash->clientSocket, master);

            deletePeerToClientHash(pHash->peerSocket);
        }
        else {
            uint16_t hashID = getHashForKey(pkt->message->key);
            int hashDestination = checkHashID(hashID, args);

            switch (hashDestination) {
                // Case 1 Der Key liegt auf dem aktuellen Server -> Anfrage ausführen und an Client schicken
                case OWN_SERVER: {
                    handleHashTableRequest(sock, pkt->message);
                    FD_CLR(sock, master);
                    close(sock);
                    break;
                }
                    // Case 2 Der Key liegt auf dem next Server -> GET/SET/DELETE an Server und Ergebnis an Client schicken
                case NEXT_SERVER: {
                    int peerSock = setupClientWithAddr(args->nextIpAddr, atoi(args->nextPort));
                    FD_SET(peerSock, master);
                    if (*(fdMax) < peerSock) {
                        *fdMax = peerSock;
                    }
                    setPeerToClientHash(peerSock, sock);

                    int sendMessageStatus = sendMessage(peerSock, pkt->message);
                    INVARIANT_CB(sendMessageStatus != -1, -1, "Failed to send Message", {
                        close(peerSock);
                    });
                    break;
                }

                    // Case 3 Der key liegt auf einem unbekannten Server -> Lookup an nächsten Server
                case UNKNOWN_SERVER: {
                    int status = setClientHash(hashID, copyMessage(pkt->message), sock);
                    if (status == -1){
                        close(sock);
                        return -1;
                    }
                    int peerSock = getpeer(hashID, args);
                    lookup *l = createLookup(0, 1,0,0,0,0,0, hashID, args->ownID, args->ownIpAddr, atoi(args->ownPort));
                    buffer *b  = encodeLookup(l);
                    sendAll(peerSock, b->buff, b->length);
                    free(b);
                    free(l);
                    close(peerSock);
                    break;
                }
            }
        }
    }
}
//own
/* -------------------------------------------------------------------------- */
/*                       //ANCHOR : STABILIZE                                 */
/* -------------------------------------------------------------------------- */
void Stabilze(serverArgs* args){ // stabilizes if time has passed
    if(joined == 1 ) {
        int peerSock = setupClientWithAddr(args->nextIpAddr, atoi(args->nextPort));
        lookup *stabilize = createLookup(0, 0, 1, 0, 0,0,0, args->ownID, args->ownID, args->ownIpAddr, atoi(args->ownPort));
        buffer*b = encodeLookup(stabilize);
        fprintf(stderr, "------------------SENDSTABILZE-----------------\n");
        printLookup(stabilize);
        sendAll(peerSock, b->buff, b->length);
        free(b);
        close(peerSock);
    }
}
/* -------------------------------------------------------------------------- */
/*                       //ANCHOR : TIME MANAGEMENT                           */
/* -------------------------------------------------------------------------- */
void twosec(serverArgs* args){
    gettimeofday(&t1, 0);
    long elapsed = (t1.tv_sec-t0.tv_sec);
    if(elapsed >= 2){
        fprintf(stderr, "timeelapsed: %ld", elapsed);
        Stabilze(args);
        gettimeofday(&t0, 0);
        tv.tv_sec = 2;
    }

}
//endown
int startServer(serverArgs *args) {
    fd_set master;    // Die Master File Deskriptoren Liste
    fd_set read_fds;  // Temporäre File Deskriptor Liste für select()
    int fdMax;        // Die maximale Anzahl an File Deskriptoren
    //own
    tv.tv_sec = 2;
    //endown

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    int socketServer = setupServer(args->ownIP, args->ownPort, &args->ownIpAddr);
    INVARIANT(socketServer != -1, -1, "");
    // Aufforderung Fingertable zu erstellen/ Aktuallisieren :)
    //own
    FingerTable * table = F_create(args->ownID);
    FTable = *table;

    args->nextIpAddr = args->ownIpAddr;
    args->prevIpAddr = args->ownIpAddr;

    FTable.first->PORT = (atoi(args->nextPort));
    FTable.first->IP = args->nextIpAddr;
    FTable.first->ID = args->nextID;
    //endown
    int listenStatus = listen(socketServer, 10);
    INVARIANT_CB(listenStatus != -1, -1, "Failed to listen for incoming requests", {
        close(socketServer);
    });

    FD_SET(socketServer, &master);
    fdMax = socketServer;
    //own
    if(ARGC == 6) {
        int knownSock = setupClient(args->knownIP, args->knownPort);
        struct sockaddr_in sa;
        inet_pton(AF_INET, args->ownIP, (&sa.sin_addr));
        lookup *stabilerhead = createLookup(0, 0, 0, 0, 1,0,0, args->ownID, args->ownID, sa.sin_addr.s_addr,atoi(args->ownPort));
        sendLookup(knownSock, stabilerhead);
        close(knownSock);
    }

    //endown
    for (;;) {
        read_fds = master; // Kopieren
        int selectStatus = select(fdMax + 1, &read_fds, NULL, NULL, &tv);
        INVARIANT(selectStatus != -1, -1, "Error in select");
        if(selectStatus == 0){
            if(joined == 1 && firstsolo == 0){
                twosec(args);
            }

        }
        for (int sock = 0; sock <= fdMax; sock++) {
            if (FD_ISSET(sock, &read_fds)) {
                if (sock == socketServer) {
                    struct sockaddr_storage their_addr;
                    socklen_t addr_size = sizeof their_addr;

                    // Accept a request from a client
                    int clientSocket = accept(socketServer, (struct sockaddr *) &their_addr, &addr_size);
                    INVARIANT_CONTINUE_CB(clientSocket != -1, "Failed to accept client", {});

                    FD_SET(clientSocket, &master);

                    if (clientSocket > fdMax) fdMax = clientSocket;

                    LOG_DEBUG("New connection");
                } else {

                    packet *pkt = recvPacket(sock);
                    INVARIANT_CONTINUE_CB(pkt != NULL, "Failed to recv message", {
                        close(sock);
                        FD_CLR(sock, &master);
                    });

                    handlePacket(pkt, sock, &master, args, &fdMax);
                    freePacket(pkt);
                    //own
                    twosec(args);
                    fprintf(stderr, "ID: %d  PORT: %s  IP: %d \n", args->ownID, args->ownPort, args->ownIpAddr);
                    fprintf(stderr, "IDP: %d  PORTP: %s  IPP: %d \n", args->prevID, args->prevPort, args->prevIpAddr);
                    fprintf(stderr, "IDN: %d  PORTN: %s  IPN: %d \n", args->nextID, args->nextPort, args->nextIpAddr);
                    printTable(&FTable);
                    //endown
                }
            }
        }
    }
}
//own
serverArgs *parseArguments(char *argv[]) {
    serverArgs *ret = calloc(1, sizeof(serverArgs));
    if(atoi(argv[3]) < 0){
        fprintf(stderr, "negative ID ");
        exit(0);
    }
    if(ARGC == 3){
        // FIRST NODE, NO ID FOUND AS INPUT
        firstsolo = 1;
        ret->ownID = 0;
    }else if(ARGC >= 4){
        // ID FOUND IN INPUT
        if(ARGC == 4){
            firstsolo = 1;
        }
        ret->ownID = atoi(argv[3]);
    }
    // INITIALIZE VALUES
    ret->ownIP = argv[1];
    ret->ownPort = argv[2];
    ret->ownIpAddr = -1;

    ret->prevID =  ret->ownID;
    ret->prevIP = NULL;
    ret->prevPort = calloc(1,6);
    memcpy(ret->prevPort , ret->ownPort,strlen(ret->ownPort)+1);

    ret->nextID =  ret->ownID;
    ret->nextIP = NULL;
    ret->nextPort = calloc(1,6);
    memcpy(ret->nextPort , ret->ownPort,strlen(ret->ownPort)+1);

    if(ARGC == 6){
        // IF JOINING NODE SAVE THE JOIN NODE DATA
        ret->knownIP = argv[4];
        ret->knownPort = argv[5];
    } else{
        ret->knownIP = NULL;
        ret->knownPort = NULL;
    }
    return ret;
}
int main(int argc, char *argv[]) {
    // Parse arguments
    ARGC = argc;
    joined = 0;
    lookupmode = 0;
    firstsolo = 0;
    serverArgs *args = parseArguments(argv);

    startServer(args);
}
//endown
