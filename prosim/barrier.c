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
    pthread_cond_init(&barr->cond1, NULL);
    pthread_cond_init(&barr->cond2, NULL);
    barr->currCondition=0;
    return barr;
}
extern void barrier_wait(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->cur_threads++;
    int temp = curr->currCondition;
    if(curr->cur_threads<curr->max_threads){
        if (temp == 0) {
            pthread_cond_wait(&curr->cond1, &curr->lock);
        } else {
            pthread_cond_wait(&curr->cond2, &curr->lock);
        }
    } else {
        curr->cur_threads=0;
        if(curr->currCondition==1){
            curr->currCondition=0;
        } else {
            curr->currCondition=1;
        }
        if (temp == 0) {
            pthread_cond_broadcast(&curr->cond1);
        } else {
            pthread_cond_broadcast(&curr->cond2);
        }
    }
//    pthread_cond_broadcast(&curr->cond);//FOR ALL THREADS
    pthread_mutex_unlock(&curr->lock);
}
extern void barrier_done(barrier_t * curr){
    pthread_mutex_lock(&curr->lock);
    curr->max_threads--;
    if(curr->currCondition==1){
        curr->currCondition=0;
    } else {
        curr->currCondition=1;
    }
    if (curr->currCondition == 0) {
        pthread_cond_broadcast(&curr->cond1);
    } else {
        pthread_cond_broadcast(&curr->cond2);
    }
    pthread_mutex_unlock(&curr->lock);
}