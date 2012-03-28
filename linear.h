#include <pthread.h>
#include <stdio.h>
#define MAX_THREADS 8

typedef struct {
  pthread_mutex_t count_lock;
  pthread_cond_t ok_to_proceed;
  int count;
} mylib_barrier_t;

void *barrier_test(void *s);
void mylib_init_barrier(mylib_barrier_t *b);
void mylib_destroy_barrier(mylib_barrier_t *b);
void mylib_barrier(mylib_barrier_t *b, int num_threads);
