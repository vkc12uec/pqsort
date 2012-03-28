#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "linear.h" // Linear Barrier library

#define FREE(x) free(x); x=NULL

/*free(nSmaller); nSmaller = NULL;*/

typedef struct thread_info
{
  int size; 
  int nthreads;  
  int pivot;  
  int tid;  

  /*arrrays used in each round */
  int *array; 
  int *tarray;  
  int *nSmaller;  
  int *nGreater; 
  int *prefix_smaller;    // these 2 hold the prefix sums
  int *prefix_larger;

  pthread_barrier_t *pbarr;
  mylib_barrier_t *barr; 
} thread_info;

// init thread_info
thread_info *init_info(int size, int *array, int *tarray, int nthreads, int *nSmaller,
    int *nGreater, int pivot, int tid, mylib_barrier_t *barr, int *prefix_smaller, int *prefix_larger, pthread_barrier_t *pbarr) {

  thread_info *temp = (thread_info *) malloc(sizeof(thread_info));
  if(!temp) return NULL;

  temp->size = size;
  temp->array = array;
  temp->tarray = tarray;
  temp->nthreads = nthreads;
  temp->nSmaller = nSmaller;
  temp->nGreater = nGreater;
  temp->pivot = pivot;
  temp->tid = tid;
  temp->barr = barr;
  temp->prefix_smaller = prefix_smaller;
  temp->prefix_larger = prefix_larger;
  temp->pbarr = pbarr;

  return temp;
}

typedef struct process_info
{
  int size;
  int *array;
  int *tarray;
  int nthreads;
} process_info;

process_info *init_pinfo(int size, int *array, int *tarray, int nthreads) {

  process_info *temp = (process_info *) malloc(sizeof(process_info));
  if(!temp) return NULL;

  temp->size = size;
  temp->array = array;
  temp->tarray = tarray;
  temp->nthreads = nthreads;

  return temp;
}

int cmpint(const void *p1, const void *p2) {
  return *((int*)p1) - *((int*)p2);
}

int *pqsort(int *array, int size, int nthreads);
void *_pqsort(void *_pinfo_data);
void *partition(void *infodata);

int total;
int total_threads;
int total_elements;

int same;
pthread_mutex_t same_mutex;
pthread_mutex_t prefix_mutex;
pthread_cond_t prefix_cv;

int *pqsort(int *array, int size, int nthreads) {

  total_elements = size;
  total_threads = nthreads;
  total = 0;

  same = 0;
  pthread_mutex_init(&same_mutex, NULL);
  pthread_mutex_init(&prefix_mutex, NULL);
  pthread_cond_init(&prefix_cv, NULL);

  // Allocate temp array for pqsort
  int *tarray = (int*) calloc(size, sizeof(int));
  if(!tarray) return NULL;

  pthread_t p;
  process_info *temp = init_pinfo(size, array, tarray, nthreads);
  pthread_create(&p, NULL, _pqsort, (void *)temp);
  pthread_join(p, NULL);

  printf("Total sub threads: %d\n\n", total);
  pthread_mutex_destroy(&same_mutex);
  pthread_mutex_destroy(&prefix_mutex);

  free(tarray); tarray = NULL;
  return array;
}

void *_pqsort(void *_pinfo_data) {

  process_info *pinfo_data = (process_info *)_pinfo_data;
  int size = pinfo_data->size;
  int *array = pinfo_data->array;
  int *tarray = pinfo_data->tarray;
  int nthreads = pinfo_data->nthreads;

  if(size <= total_elements/total_threads) {
    qsort(array, size, sizeof(int), cmpint);
    pthread_exit(NULL);
  }

  // Select first element of the input array as the pivot
  int pivot = array[0];

  int *nSmaller = (int *) calloc(nthreads, sizeof(int));
  if(!nSmaller) pthread_exit(NULL);

  int *nGreater = (int *) calloc(nthreads, sizeof(int));
  if(!nGreater) pthread_exit(NULL);

  int *prefix_smaller = (int *) calloc(nthreads+1, sizeof(int));    // becoz it start with 0
  if(!prefix_smaller) pthread_exit(NULL);

  int *prefix_larger = (int *) calloc(nthreads+1, sizeof(int));   // becoz it start with 0
  if(!prefix_larger) pthread_exit(NULL);

  pthread_t threads[nthreads];

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  mylib_barrier_t barr;
  mylib_init_barrier(&barr);

  pthread_barrier_t pbarr;
  pthread_barrier_init(&pbarr, NULL, nthreads);

  int rc, tid = 0;
  for(tid = 0; tid < nthreads; tid++) {
    thread_info *temp = init_info(size, array, tarray, nthreads, nSmaller, nGreater, pivot, tid, &barr, prefix_smaller, prefix_larger, &pbarr);
    rc = pthread_create(&threads[tid], &attr, partition, (void *)temp);

    pthread_mutex_lock(&same_mutex);
    total++;
    pthread_mutex_unlock(&same_mutex);

    if (rc) pthread_exit(NULL);
  }

  int i;
  for (i = 0; i < nthreads ; i++)
    pthread_join(threads[i], NULL);

  // swap pivot and right boundry of small
  int small = prefix_smaller[nthreads]; //nSmaller[nthreads-1];
  int greater = prefix_larger[nthreads];  //nGreater[nthreads-1];


  if(small-1) {
#ifdef DEBUG
    printf("\nBASE : all sub-threads joined, small = %d, greater = %d", small, greater);
    printf ("\n before swapping with pivot, rearrange array = ");
    for (i=0; i<size; i++){
      printf ("\t %d", array[i]);
    }

    printf ("\nswapping positions %d with %d" , 0, small-1);
#endif

    array[0] = array[0] ^ array[small-1];
    array[small-1] = array[0] ^ array[small-1];
    array[0] = array[0] ^ array[small-1];

  }

  #if 0
  free(nSmaller); nSmaller = NULL;
  free(nGreater); nGreater = NULL;
  free(prefix_smaller); prefix_smaller = NULL;
  free(prefix_larger); prefix_larger = NULL;
  #endif
  FREE(nSmaller);
  FREE(nGreater);
  FREE(prefix_smaller);
  FREE(prefix_larger);

  pthread_attr_destroy(&attr);
  mylib_destroy_barrier(&barr);

  // Recursion
  pthread_t thread[2];

  int nthreads_s = (nthreads*small) / size;
  int nthreads_g = nthreads - nthreads_s;

  assert((nthreads_s+nthreads_g) == nthreads);

  process_info *temp1 = init_pinfo(small-1, array, tarray, nthreads_s?nthreads_s:1);
  pthread_create(&thread[0], NULL, _pqsort, (void *)temp1);

  process_info *temp2 = init_pinfo(greater, array+small, tarray+small, nthreads_g);
  pthread_create(&thread[1], NULL, _pqsort, (void *)temp2);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);

  /*if(small-1) {
    pthread_t p;
    pinfo *temp1 = init_pinfo(small-1, array, tarray, nthreads);
    pthread_create(&p, NULL, _pqsort, (void *)temp1);
    pthread_join(p, NULL);
    }
    if(greater){
    pthread_t p;
    pinfo *temp2 = init_pinfo(greater, array+small, tarray+small, nthreads);
    pthread_create(&p, NULL, _pqsort, (void *)temp2);
    pthread_join(p, NULL);
    }*/

  free(pinfo_data); pinfo_data = NULL;
  pthread_exit(NULL);
}

void *partition(void *_infodata) {

  thread_info *infodata = (thread_info *) _infodata;
  int tid = infodata->tid;

  int ws = ceil((double)infodata->size / infodata->nthreads);
  int start = ws * tid;
  int end = ws * (tid+1);

  if(end > infodata->size) 
    end = infodata->size;

  int lesser = start;
  int greater = end-1;

  int i;

  /*quicksort*/
  for(i = start; i < end; i++) {
    /*if(infodata->array[i] == infodata->pivot) {
      pthread_mutex_lock (&same_mutex);
      same++;
      pthread_mutex_unlock (&same_mutex);
      }*/

    if(infodata->array[i] <= infodata->pivot)
      infodata->tarray[lesser++] = infodata->array[i];
    else
      infodata->tarray[greater--] = infodata->array[i];
  }

  infodata->nSmaller[tid] = lesser - start;
  infodata->nGreater[tid] = end - greater -1;

  #ifdef DEBUG
  if (tid == 0) {
    printf ("\ntid: 0 after internal quicksort");
    for (i=start; i<end; i++)
      printf ("\ntid: %d .... i = %d, value = %d ", tid , i, infodata->tarray[i]);
  }
  #endif

  // serial naveen's sum
  // T0: propagates the prefix sum forward.....other wait for him in barrier
  // no. of barriers used = 3
  //mylib_barrier(infodata->barr, infodata->nthreads);

  pthread_barrier_wait (infodata->pbarr);

  /*printf ("\ntid: %d ---> after 1st barrier ", tid);*/

  if (tid == 0) {
    // I shud do the sum and propagate
    // modify infodata->prefix_smaller and join the barrier

    #ifdef DEBUG
    printf ("\nT0: doing reaarnge");
    printf ("\nT0: looking at whole nSmaller[] and nGreater[] ");

    for (i=0; i<infodata->nthreads; i++)
      printf ("\nt0: i = %d | nSmaller = %d | nGreater = %d", i, infodata->nSmaller[i], infodata->nGreater[i]);
    #endif

    infodata->prefix_smaller[0] = 0;
    int j=1;
    for (; j<= infodata->nthreads; j++)
      infodata->prefix_smaller[j] = infodata->prefix_smaller[j-1] + infodata->nSmaller[j-1];

    // modify infodata->prefix_larger

    infodata->prefix_larger[0] = 0;
    j=1;
    for (; j<= infodata->nthreads; j++)
      infodata->prefix_larger[j] = infodata->prefix_larger[j-1] + infodata->nGreater[j-1];

    #ifdef DEBUG
    printf ("\nT0: @@ debug prefix_smaller prefix_larger arrays");
    for (j=0; j<= infodata->nthreads; j++)
      printf ("\nT0: j = %d | prefix_smaller = %d | prefix_larger = %d", j, infodata->prefix_smaller[j], infodata->prefix_larger[j]);

    #endif

    pthread_barrier_wait (infodata->pbarr);
    /*printf ("\nT0: @@ bcacst done");*/
  }
  else {
    /*printf ("\ntid %d: ---> start waiting for bcast", tid);*/
    pthread_barrier_wait (infodata->pbarr);
    /*printf ("\ntid %d: --> end 2nd bcast", tid);*/
  }

  // all threads can now access global prefix_smaller and prefix_larger
  int offsetSmaller = infodata->prefix_smaller[tid];
  int offsetLarger = infodata->prefix_smaller[infodata->nthreads] + infodata->prefix_larger[tid];

  /*printf ("\ntid: %d offsetSmaller = %d , offsetLarger = %d", tid, offsetSmaller, offsetLarger);*/

  //rearrange 
  for(i = start; i < end; i++) {
    if (infodata->tarray[i] <= infodata->pivot)
      infodata->array[offsetSmaller++] = infodata->tarray[i];
    else
      infodata->array[offsetLarger++] = infodata->tarray[i];
  }

  #ifdef DEBUG
    
    pthread_barrier_wait (infodata->pbarr);

    // now if order is ok, then both t1 t0 are rearrenging ok.

  // try to print infodata->array here
  if (tid == 0) {
    printf ("\nT0: printing the rearranged array ");
    for(i = 0; i < infodata->size; i++) {
      printf ("\n ** %d", infodata->array[i]);
    }
    pthread_barrier_wait (infodata->pbarr);
  }
  else {
    pthread_barrier_wait (infodata->pbarr);
  }

  #endif

  /*printf ("\ninfinite wait after 1st stage");*/
  /*pthread_cond_wait (&prefix_cv, &prefix_mutex);*/

#if 0
  // Compute prefix sum
  int j, sbackup, gbackup;
  for(j = 0; j < ceil(log(infodata->nthreads)/log(2)); j++) {
    mylib_barrier(infodata->barr, infodata->nthreads);
    if((tid - (1<<j)) >= 0) sbackup = infodata->nSmaller[tid - (1<<j)];
    if((tid - (1<<j)) >= 0) gbackup = infodata->nGreater[tid - (1<<j)];
    mylib_barrier(infodata->barr, infodata->nthreads);

    if((tid - (1<<j)) >= 0) {
      infodata->nSmaller[tid] += sbackup;
      infodata->nGreater[tid] += gbackup;
    }
  }
  mylib_barrier(infodata->barr, infodata->nthreads);

  // Computing final indexes for smaller and greater values
  int count, countb;
  count = tid ? infodata->nSmaller[tid-1] : 0;
  countb = tid ? infodata->nGreater[tid-1] : 0;

  for(i = start; i < end; i++) {
    if(infodata->tarray[i] <= infodata->pivot)
      infodata->array[count++] = infodata->tarray[i];
    else
      infodata->array[infodata->size - countb++ - 1] = infodata->tarray[i];
  }
#endif

  free(infodata); infodata = NULL;
  pthread_exit(NULL);
  //return NULL;
}
