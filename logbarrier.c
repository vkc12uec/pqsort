
#include "logbarrier.h"


#if 0
#ifdef ALONE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MAX_THREADS 16
void *barrier_test(void *s);
#endif

typedef struct barrier_node {
  pthread_mutex_t count_lock;
  pthread_cond_t ok_to_proceed_up;
  pthread_cond_t ok_to_proceed_down;
  int count;
} mylib_barrier_t_internal;

typedef struct barrier_node mylob_logbarrier_t;    //[MAX_THREADS];

#ifdef ALONE
pthread_t p_threads[MAX_THREADS];
pthread_attr_t attr;
#endif

#endif

// uncomment if u use below 2 in self main file 
pthread_t p_threads[MAX_THREADS];
pthread_attr_t attr;

mylob_logbarrier_t  *mylib_init_barrier (int max_threads)
  /*void mylib_init_barrier(mylob_logbarrier_t b)*/
{
  //mylob_logbarrier_t *b = *pb;
  mylob_logbarrier_t *b;
  b = (struct barrier_node *) malloc (sizeof (struct barrier_node) * max_threads);

  if (!b) {
    printf ("\nbad idea");
    //assert(0);
  }

  int i;
  for (i = 0; i < max_threads; i++) {
    //for (i = 0; i < MAX_THREADS; i++) {
    b[i].count = 0;
    pthread_mutex_init(&(b[i].count_lock), NULL);
    pthread_cond_init(&(b[i].ok_to_proceed_up), NULL);
    pthread_cond_init(&(b[i].ok_to_proceed_down), NULL);
  }

  //memcpy ( (void*)&pb, (void*)b, sizeof (struct barrier_node) * max_threads);
  return b;
  }

  void mylib_logbarrier (mylob_logbarrier_t *b, int num_threads, int thread_id)
  {
    int i, base, index;
    i = 2;
    base = 0;

    do {
      index = base + thread_id / i;
      if (thread_id % i == 0) {
        pthread_mutex_lock(&(b[index].count_lock));
        b[index].count ++;
        while (b[index].count < 2)
          pthread_cond_wait(&(b[index].ok_to_proceed_up),
              &(b[index].count_lock));
        pthread_mutex_unlock(&(b[index].count_lock));
      }
      else {
        pthread_mutex_lock(&(b[index].count_lock));
        b[index].count ++;
        if (b[index].count == 2)
          pthread_cond_signal(&(b[index].ok_to_proceed_up));
        /*
           while (b[index].count != 0)
         */
        while (
            pthread_cond_wait(&(b[index].ok_to_proceed_down),
              &(b[index].count_lock)) != 0);
        pthread_mutex_unlock(&(b[index].count_lock));
        break;
      }
      base = base + MAX_THREADS/i;
      i = i * 2; 
    } while (i <= MAX_THREADS);

    i = i / 2;

    for (; i > 1; i = i / 2)
    {
      base = base - MAX_THREADS/i;
      index = base + thread_id / i;
      pthread_mutex_lock(&(b[index].count_lock));
      b[index].count = 0;
      pthread_cond_signal(&(b[index].ok_to_proceed_down));
      pthread_mutex_unlock(&(b[index].count_lock));
    }
  }

//#ifdef ALONE
  mylob_logbarrier_t *barr;
  main()
  {
    /* barrier tester */
    int i, hits[MAX_THREADS];

    pthread_attr_init (&attr);
    pthread_attr_setscope (&attr,PTHREAD_SCOPE_SYSTEM);

    barr = mylib_init_barrier (MAX_THREADS);
    printf ("\nI am ok");

    for (i=0; i< MAX_THREADS; i++)
    {
      hits[i] = i;
      pthread_create(&p_threads[i], &attr, barrier_test, (void *) &hits[i]);
    }
    for (i=0; i< MAX_THREADS; i++)
      pthread_join(p_threads[i], NULL);

  }


  void *barrier_test(void *s)
  {
    int thread_id, i, j;
    thread_id = *((int *) s);
    printf ("\n in barr - test");
    for (i=0; i < 100; i++)
      mylib_logbarrier(barr, MAX_THREADS, thread_id);
  }
//#endif
