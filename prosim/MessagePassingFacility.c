//
// Created by Krutik on 2024-08-05.
//

#include "MessagePassingFacility.h"
extern void facilityInit(MessageFacility *messageFacility){
    messageFacility->recvQ=*prio_q_new();
    messageFacility->sendQ=*prio_q_new();
    pthread_mutex_init(&messageFacility->lock, NULL);
}
extern void send(MessageFacility *messageFacility,context *sender, int nodeRecv, int procRecv){
    pthread_mutex_lock(&messageFacility->lock);
    prio_q_t temp = *prio_q_new();
    int found = 0;
    while(!prio_q_empty(&messageFacility->recvQ)){
        context *cur = prio_q_remove(&messageFacility->recvQ);
        if(cur->thread==nodeRecv && cur->id==procRecv){
            printf("Sended\n");
            found = 1;
            break;
        }
        prio_q_add(&temp,cur,cur->id);
    }
    if(found==1){

    } else {
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
        if(cur->thread==nodeSend && cur->id==procSend){
            printf("Received\n");
            found = 1;
            break;
        }
        prio_q_add(&temp,cur,cur->id);
    }
    if(found==1){

    } else {
        prio_q_add(&messageFacility->recvQ,receiver,receiver->id);
    }
    while(!prio_q_empty(&temp)){
        context *cur = prio_q_remove(&temp);
        prio_q_add(&messageFacility->sendQ,cur,cur->id);
    }
    pthread_mutex_unlock(&messageFacility->lock);
}