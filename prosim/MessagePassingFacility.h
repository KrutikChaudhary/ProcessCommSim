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
    prio_q_t sendQ;
    prio_q_t recvQ;
    prio_q_t completed;
    pthread_mutex_t lock;
}MessageFacility_t;

extern void facilityInit(MessageFacility_t **messageFacility);
extern void send(MessageFacility_t *messageFacility, context *sender, int nodeRecv, int procRecv);
extern void recv(MessageFacility_t *messageFacility, context *receiver, int nodeSend, int procSend);