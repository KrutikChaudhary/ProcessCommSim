//
// Created by Krutik on 2024-08-04.
//

#ifndef PROSIM_BARRIER_H
#define PROSIM_BARRIER_H

#endif //PROSIM_BARRIER_H
#include <pthread.h>
typedef struct barrier_t {
    int max_threads;
    int cur_threads;
    pthread_mutex_t lock;
    pthread_cond_t cond1;
    pthread_cond_t cond2;
    int currCondition;
} barrier_t;
extern barrier_t *barrier_new(int n);
extern void barrier_wait(barrier_t * curr);
extern void barrier_done(barrier_t * curr);