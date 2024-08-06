//
// Created by Krutik on 2024-08-05.
//

#ifndef PROSIM_MESSAGEPASSINGFACILITY_H
#define PROSIM_MESSAGEPASSINGFACILITY_H

#endif //PROSIM_MESSAGEPASSINGFACILITY_H
#include "prio_q.h"
#include "context.h"

typedef struct MessageFacility {
    prio_q_t sendQ;
    prio_q_t recvQ;
}MessageFacility;

extern void facilityInit(MessageFacility *messageFacility);
extern void send(MessageFacility *messageFacility,context *sender, int nodeRecv, int procRecv);
extern void recv(MessageFacility *messageFacility, context *receiver, int nodeSend, int procSend);