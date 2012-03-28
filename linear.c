#include "linear.h"

void mylib_init_barrier(mylib_barrier_t *b)
{
  b -> count = 0;
  pthread_mutex_init(&(b -> count_lock), NULL);
  pthread_cond_init(&(b -> ok_to_proceed), NULL);
}

void mylib_destroy_barrier(mylib_barrier_t *b)
{
  pthread_mutex_destroy(&(b -> count_lock));
  pthread_cond_destroy(&(b -> ok_to_proceed));
}

void mylib_barrier (mylib_barrier_t *b, int num_threads)
{
  pthread_mutex_lock(&(b -> count_lock));
  b -> count ++;
  if (b -> count == num_threads) {
    b -> count = 0;
    pthread_cond_broadcast(&(b -> ok_to_proceed));
  }
  else
    while (pthread_cond_wait(&(b -> ok_to_proceed), &(b -> count_lock)) != 0);
  pthread_mutex_unlock(&(b -> count_lock));
}

/*pthread_t p_threads[MAX_THREADS];
pthread_attr_t attr;

mylib_barrier_t barr;

main()
{
  int i, hits[MAX_THREADS];

  pthread_attr_init (&attr);
  pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);

  mylib_init_barrier (&barr);

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
  printf("tid=%d\n", thread_id);
  mylib_barrier(&barr, MAX_THREADS);
  printf("tid=%d\n", thread_id);
}*/
