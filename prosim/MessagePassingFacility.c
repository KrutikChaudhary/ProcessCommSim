//
// Created by Krutik on 2024-08-05.
//
#include <stdlib.h>
#include "MessagePassingFacility.h"

//function to initialize variables and data structures
extern void facilityInit(MessageFacility_t **messageFacility) {
    *messageFacility = (MessageFacility_t *)calloc(1, sizeof(MessageFacility_t));
    if (*messageFacility == NULL) {
        printf("Failed to memory calloc\n");
        return;
    }
    (*messageFacility)->recvQ = *prio_q_new();
    (*messageFacility)->sendQ = *prio_q_new();
    (*messageFacility)->completed = *prio_q_new();

    pthread_mutex_init(&(*messageFacility)->lock, NULL);
}

//function to simulate message sending
extern void send(MessageFacility_t *messageFacility, context *sender, int nodeRecv, int procRecv){
    pthread_mutex_lock(&messageFacility->lock); //lock
    prio_q_t temp = *prio_q_new(); //make a temp queue
    int found = 0;

    //find if receiver is waiting
    while(!prio_q_empty(&messageFacility->recvQ)){
        context *cur = prio_q_remove(&messageFacility->recvQ);

        //if sender-receiver message passing addresses matches
        if(cur->thread==nodeRecv && cur->id==procRecv && cur->code[cur->ip].addressNodeId== sender->thread && cur->code[cur->ip].addressProcessId==sender->id){
            //receiver is found waiting
            found = 1;

            //add both sender and receiver in completed message passing queue
            prio_q_add(&messageFacility->completed,sender,sender->id);
            prio_q_add(&messageFacility->completed,cur,cur->id);
            break;
        }

        //no match add to temp
        prio_q_add(&temp,cur,cur->id);
    }
    if(found!=1){
        //if not found, make sender wait, until receiver joins
        prio_q_add(&messageFacility->sendQ,sender,sender->id);
    }
    while(!prio_q_empty(&temp)){ //add all temp values back to recv queue as it is
        context *cur = prio_q_remove(&temp);
        prio_q_add(&messageFacility->recvQ,cur,cur->id);
    }

    //unlock
    pthread_mutex_unlock(&messageFacility->lock);
}

//function to simulate message recv
extern void recv(MessageFacility_t *messageFacility, context *receiver, int nodeSend, int procSend){
    pthread_mutex_lock(&messageFacility->lock); //lock

    prio_q_t temp = *prio_q_new(); //temporary queue
    int found = 0; //variable to determine if sender is found or not

    while(!prio_q_empty(&messageFacility->sendQ)){ //find if sender is waiting in the sender queue
        context *cur = prio_q_remove(&messageFacility->sendQ);

        //if sender-receiver message passing addresses matches, then sender is found
        if(cur->thread==nodeSend && cur->id==procSend && cur->code[cur->ip].addressNodeId==receiver->thread && cur->code[cur->ip].addressProcessId==receiver->id){
            found = 1; //set to found

            //add both of them in completed message passing queue
            prio_q_add(&messageFacility->completed,receiver,receiver->id);
            prio_q_add(&messageFacility->completed,cur,cur->id);
            break;
        }

        //no match found add to temp, to add back to original queue
        prio_q_add(&temp,cur,cur->id);
    }
    if(found!=1){//not found add to receive queue to wait for future sender
        prio_q_add(&messageFacility->recvQ,receiver,receiver->id);
    }

    //add temp elements back as it is to send queue
    while(!prio_q_empty(&temp)){
        context *cur = prio_q_remove(&temp);
        prio_q_add(&messageFacility->sendQ,cur,cur->id);
    }

    //unlock
    pthread_mutex_unlock(&messageFacility->lock);
}