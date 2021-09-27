//own
#include <stdlib.h>
#include <stdio.h>
#include "../include/Fingertable.h"

int power(int base, int exp)
{
    int result = 1;
    while(exp) { result *= base; exp--; }
    return result;
}

struct FingerTable * F_create(uint16_t id) {
    // ein neues struct des typen prio_q wird generiert;
    struct FingerTable* temp = (struct FingerTable*)malloc(sizeof(struct FingerTable));
    temp->size = 0;	// init size mit 0
    temp-> first = NULL; // first ptr ist NULL zu beginn da kein elem drin ist
    temp->curr = NULL;
    while(temp->size < 16){
        uint16_t ID = (id+power(2,temp->size))%(power(2, 16));
        struct Finger * new = malloc(sizeof(struct Finger));// alloc elem
        new-> startID = ID;// weise werte zu
        new->next_ptr= NULL;// next ptr auch 0 vorerst
        temp->size = temp->size+1;// liste wird um ein elem vergrößert darum size +1
        if(temp->curr != NULL){
            temp->curr->next_ptr = new;
            temp->curr = temp->curr->next_ptr;
        }
        // List leer -> new elem wird head
        if(temp->first == NULL){
            temp->first = new;
            temp->curr = new ;
        }
    }
    temp->curr = temp->first;
    return temp;
}

void F_destroy(struct FingerTable *q) {
    while ( q->first != NULL ){	//solange die liste nicht leer ist do gib elem frei
        struct Finger * tmp = q->first;	//ptr speichern
        q->first = q->first->next_ptr;	//ptr = ptr next
        free ( tmp );					//gebe tmp frei bzw ptr
    }
    free(q);						//gebe die liste frei
}

void printTable(struct FingerTable * q) {
    fprintf(stderr, "\n-------------------FINGERTABLE---------------------\n");
    int u = 0;

    while (q->curr != NULL) {
        fprintf(stderr, "NR: %d STARTID: %d  ID: %d  IP: %d  PORT: %d\n", u, q->curr->startID, q->curr->ID, q->curr->IP,
                q->curr->PORT);
        u++;
        q->curr = q->curr->next_ptr;
    }
    fprintf(stderr, "\n-------------------FINGERTABLEENDE---------------------\n");
    q->curr = q->first;
}
//endown