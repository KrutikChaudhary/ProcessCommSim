//
// Created by Krutik on 2024-08-05.
//
#include <stdlib.h>
#include "MessagePassingFacility.h"
extern void facilityInit(MessageFacility **messageFacility) {
    *messageFacility = (MessageFacility *)calloc(1, sizeof(MessageFacility));
    if (*messageFacility == NULL) {
        printf("Failed to memory calloc\n");
        return;
    }
    (*messageFacility)->recvQ = *prio_q_new();
    (*messageFacility)->sendQ = *prio_q_new();
    (*messageFacility)->completed = *prio_q_new();

    pthread_mutex_init(&(*messageFacility)->lock, NULL);
}
extern void send(MessageFacility *messageFacility,context *sender, int nodeRecv, int procRecv){
    pthread_mutex_lock(&messageFacility->lock);
    prio_q_t temp = *prio_q_new();
    int found = 0;
    while(!prio_q_empty(&messageFacility->recvQ)){
        context *cur = prio_q_remove(&messageFacility->recvQ);
        if(cur->thread==nodeRecv && cur->id==procRecv && cur->code[cur->ip].addressNodeId== sender->thread && cur->code[cur->ip].addressProcessId==sender->id){
            //printf("Sended\n");
            found = 1;
            prio_q_add(&messageFacility->completed,sender,sender->id);
            prio_q_add(&messageFacility->completed,cur,cur->id);
            break;
        }
        prio_q_add(&temp,cur,cur->id);
    }
    if(found!=1){
        //printf("Sender blocked id %d %d send\n",sender->id, procRecv);
        prio_q_add(&messageFacility->sendQ,sender,sender->id);
    }
    while(!prio_q_empty(&temp)){
        context *cur = prio_q_remove(&temp);
        prio_q_add(&messageFacility->recvQ,cur,cur->id);
    }

    pthread_mutex_unlock(&messageFacility->lock);
}
extern void recv(MessageFacility *messageFacility, context *receiver, int nodeSend, int procSend){
    pthread_mutex_lock(&messageFacility->lock);
    prio_q_t temp = *prio_q_new();
    int found = 0;
    while(!prio_q_empty(&messageFacility->sendQ)){

        context *cur = prio_q_remove(&messageFacility->sendQ);
        //printf("fsdf %d %d %d %d\n", cur->thread,cur->id,nodeSend,procSend);
        if(cur->thread==nodeSend && cur->id==procSend && cur->code[cur->ip].addressNodeId==receiver->thread && cur->code[cur->ip].addressProcessId==receiver->id){
            //printf("Received\n");
            found = 1;
            prio_q_add(&messageFacility->completed,receiver,receiver->id);
            prio_q_add(&messageFacility->completed,cur,cur->id);
            break;
        }
        prio_q_add(&temp,cur,cur->id);
    }
    if(found!=1){
        //printf("not found\n");
        prio_q_add(&messageFacility->recvQ,receiver,receiver->id);
    } else {

    }
    while(!prio_q_empty(&temp)){
        context *cur = prio_q_remove(&temp);
        prio_q_add(&messageFacility->sendQ,cur,cur->id);
    }
    pthread_mutex_unlock(&messageFacility->lock);
    //printf("received\n");
}