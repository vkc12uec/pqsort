
#ifndef _LOGBARR_H
#define _LOGBARR_H


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_THREADS 8
void *barrier_test(void *s);

typedef struct barrier_node {
  pthread_mutex_t count_lock;
  pthread_cond_t ok_to_proceed_up;
  pthread_cond_t ok_to_proceed_down;
  int count;
} mylib_barrier_t_internal;

typedef struct barrier_node mylob_logbarrier_t;    //[MAX_THREADS];

mylob_logbarrier_t  *mylib_init_barrier(int max_threads);
void mylib_logbarrier (mylob_logbarrier_t *b, int num_threads, int thread_id);

#endif
