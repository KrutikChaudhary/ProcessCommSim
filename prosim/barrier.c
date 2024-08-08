//
// Created by Krutik on 2024-08-04.
//
#include "barrier.h"
#include <stdlib.h>
extern barrier_t *barrier_new(int n){
    barrier_t * barr= calloc(1, sizeof(barrier_t));
    barr->cur_threads=0;
    barr->max_threads=n;
    pthread_mutex_init(&barr->lock, NULL);
    pthread_cond_init(&barr->cond, NULL);
    return barr;
}
extern void barrier_wait(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->cur_threads++;
    if(curr->cur_threads<curr->max_threads){
        pthread_cond_wait(&curr->cond, &curr->lock);
    } else {
        curr->cur_threads=0;
    }
    pthread_cond_broadcast(&curr->cond);//FOR ALL THREADS
    pthread_mutex_unlock(&curr->lock);
}
extern void barrier_done(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->max_threads--;
    if (curr->cur_threads == curr->max_threads) {
        curr->cur_threads = 0;
        pthread_cond_broadcast(&curr->cond);//FOR ALL THREADS
    }
    pthread_mutex_unlock(&curr->lock);
}