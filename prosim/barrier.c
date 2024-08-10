//
// Created by Krutik on 2024-08-04.
//
#include "barrier.h"
#include <stdlib.h>

//function to initialize the variables of barrier
extern barrier_t *barrier_new(int n){
    barrier_t * barr= calloc(1, sizeof(barrier_t));
    barr->cur_threads=0;
    barr->max_threads=n;
    pthread_mutex_init(&barr->lock, NULL);
    pthread_cond_init(&barr->cond, NULL);
    return barr;
}

//function to make threads wait until all of them are together
extern void barrier_wait(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->cur_threads++; //increment
    if(curr->cur_threads<curr->max_threads){
        pthread_cond_wait(&curr->cond, &curr->lock);//wait for others
    } else {
        curr->cur_threads=0;
    }
    pthread_cond_broadcast(&curr->cond);//signal for all threads
    pthread_mutex_unlock(&curr->lock);
}

//function to decrement the max_threads if that thread doesnt need barrier anymore(finished simulation)
extern void barrier_done(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->max_threads--; //decrement
    if (curr->cur_threads == curr->max_threads) {
        curr->cur_threads = 0; //set to zero, no one in wait zone
        pthread_cond_broadcast(&curr->cond);//signal for all threads
    }
    pthread_mutex_unlock(&curr->lock);
}