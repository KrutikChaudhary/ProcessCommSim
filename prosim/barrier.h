//
// Created by Krutik on 2024-08-04.
//

#ifndef PROSIM_BARRIER_H
#define PROSIM_BARRIER_H

#endif //PROSIM_BARRIER_H
#include <pthread.h>
typedef struct barrier_s {
    int max_threads;//store max threads currently for barrier
    int cur_threads;//current threads inside wait area of barrier

    //lock and condition variable to manage critical sections and condition satisfaction
    pthread_mutex_t lock;
    pthread_cond_t cond;
} barrier_t;

//function to initialize the variables of barrier
extern barrier_t *barrier_new(int n);

//function to make threads wait until all of them are together
extern void barrier_wait(barrier_t * curr);

//function to decrement the max_threads if that thread doesnt need barrier anymore(finished simulation)
extern void barrier_done(barrier_t * curr);