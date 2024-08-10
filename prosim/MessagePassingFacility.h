//
// Created by Krutik on 2024-08-05.
//

#ifndef PROSIM_MESSAGEPASSINGFACILITY_H
#define PROSIM_MESSAGEPASSINGFACILITY_H

#endif //PROSIM_MESSAGEPASSINGFACILITY_H
#include "prio_q.h"
#include "context.h"
#include <pthread.h>
typedef struct MessageFacility {
    //to store blocked send and recv processes
    prio_q_t sendQ;
    prio_q_t recvQ;

    //for processes that have just finished their send and recv funtion
    prio_q_t completed;

    //lock for critical sections
    pthread_mutex_t lock;
}MessageFacility_t;

//function to initialize variables and data structures
extern void facilityInit(MessageFacility_t **messageFacility);

//function to simulate message sending
extern void send(MessageFacility_t *messageFacility, context *sender, int nodeRecv, int procRecv);

//function to simulate message recv
extern void recv(MessageFacility_t *messageFacility, context *receiver, int nodeSend, int procSend);