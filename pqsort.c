#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "linear.h" // Linear Barrier library

typedef struct info
{
  int size; // Size of the input array
  int *array; // Input data array
  int *tarray;  // Temp data array
  int nthreads;  // Number of number threads
  int *nSmaller;  // Number of elements smaller than pivot in each chunk
  int *nGreater; // Number of elements greater than pivot in each chunk
  int pivot;  // Pivot value
  int tid;  // Thread id
  mylib_barrier_t *barr; // Barrier
  int *prefix_smaller;
  int *prefix_larger;
  pthread_barrier_t *pbarr;
} info;

// Initialize info
info *init_info(int size, int *array, int *tarray, int nthreads, int *nSmaller,
    int *nGreater, int pivot, int tid, mylib_barrier_t *barr, int *prefix_smaller, int *prefix_larger, pthread_barrier_t *pbarr) {

  info *temp = (info *) malloc(sizeof(info));
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

typedef struct pinfo
{
  int size;
  int *array;
  int *tarray;
  int nthreads;
} pinfo;

pinfo *init_pinfo(int size, int *array, int *tarray, int nthreads) {

  pinfo *temp = (pinfo *) malloc(sizeof(pinfo));
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
int total_t;
int total_s;

int same;
pthread_mutex_t same_mutex;
pthread_mutex_t prefix_mutex;
pthread_cond_t prefix_cv;

/*int main(int argc, char *argv[]) {

  // Number of threads
  int nthreads = 1;
  if(argc > 1) nthreads = atoi(argv[1]);
  printf("Number of threads = %d\n", nthreads);

  // Number of inputs
  int size = 0;
  if(argc > 2) size = atoi(argv[2]);
  printf("Size = %d\n", size);

  // Construct input array
  int *array = (int*) malloc(size * sizeof(int));
  if(!array) return -2;

  int n = size;
  srand(time(NULL));
  while(n--)
    array[n] = rand();

  // Print the unsorted array
  /*int i;
    printf("\nUnsorted\n");
    for(i = 0; i < size; i++) {
    printf("%d\n", array[i]);
    }

  struct timeval tim;
  gettimeofday(&tim,NULL);
  double start = tim.tv_sec + (tim.tv_usec/1000000.0);

  pqsort(array, size, nthreads);

  gettimeofday(&tim,NULL);
  double end = tim.tv_sec + (tim.tv_usec/1000000.0);
  printf("Time taken: %f\n\n", end - start);

  // Print the sorted array
  /*printf("\nSorted\n");
    for(i = 0; i < size; i++) {
    printf("%d\n", array[i]);
    }

  free(array); array = NULL;
  return 0;
}*/

int *pqsort(int *array, int size, int nthreads) {

  total_s = size;
  total_t = nthreads;
  total = 0;

  same = 0;
  pthread_mutex_init(&same_mutex, NULL);
  pthread_mutex_init(&prefix_mutex, NULL);
  pthread_cond_init(&prefix_cv, NULL);

  // Allocate temp array for pqsort
  int *tarray = (int*) calloc(size, sizeof(int));
  if(!tarray) return NULL;

  pthread_t p;
  pinfo *temp = init_pinfo(size, array, tarray, nthreads);
  pthread_create(&p, NULL, _pqsort, (void *)temp);
  pthread_join(p, NULL);

  printf("Total sub threads: %d\n\n", total);
  pthread_mutex_destroy(&same_mutex);
  pthread_mutex_destroy(&prefix_mutex);

  free(tarray); tarray = NULL;
  return array;
}

void *_pqsort(void *_pinfo_data) {

  pinfo *pinfo_data = (pinfo *)_pinfo_data;
  int size = pinfo_data->size;
  int *array = pinfo_data->array;
  int *tarray = pinfo_data->tarray;
  int nthreads = pinfo_data->nthreads;

  if(size <= total_s/total_t) {
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
    info *temp = init_info(size, array, tarray, nthreads, nSmaller, nGreater, pivot, tid, &barr, prefix_smaller, prefix_larger, &pbarr);
    rc = pthread_create(&threads[tid], &attr, partition, (void *)temp);

    /*pthread_mutex_lock(&same_mutex);*/
    /*total++;*/
    /*pthread_mutex_unlock(&same_mutex);*/

    if (rc) pthread_exit(NULL);
  }

  // Wait to join all the threads
  int i;
  for (i = 0; i < nthreads ; i++)
    pthread_join(threads[i], NULL);

  //printf("Same element = %d\n", same);
  //same = 0;

  // Move pivot to the center position
  int small = nSmaller[nthreads-1];
  int greater = nGreater[nthreads-1];

  if(small-1) {
    array[0] = array[small-1] + array[0];
    array[small-1] = array[0] - array[small-1];
    array[0] = array[0] - array[small-1];
  }

  free(nSmaller); nSmaller = NULL;
  free(nGreater); nGreater = NULL;
  free(prefix_smaller); prefix_smaller = NULL;
  free(prefix_larger); prefix_larger = NULL;

  pthread_attr_destroy(&attr);
  mylib_destroy_barrier(&barr);

  // Recursion
  pthread_t thread[2];

  int nthreads_s = (nthreads*small) / size;
  int nthreads_g = nthreads - nthreads_s;

  assert((nthreads_s+nthreads_g) == nthreads);

  pinfo *temp1 = init_pinfo(small-1, array, tarray, nthreads_s?nthreads_s:1);
  pthread_create(&thread[0], NULL, _pqsort, (void *)temp1);

  pinfo *temp2 = init_pinfo(greater, array+small, tarray+small, nthreads_g);
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

  info *infodata = (info *) _infodata;
  int tid = infodata->tid;

  int ws = ceil((double)infodata->size / infodata->nthreads);
  int start = ws * tid;
  int end = ws * (tid+1);

  if(end > infodata->size) 
    end = infodata->size;

  int lesser = start;
  int greater = end-1;

  int i;
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

  // serial naveen's sum
  //mylib_barrier(infodata->barr, infodata->nthreads);
  pthread_barrier_wait (infodata->pbarr);

  printf ("\ntid: %d ---> after 1st barrier ", tid);

  if (tid == 0) {
    // I shud do the sum and propagate
    // modify infodata->prefix_smaller and join the barrier

    printf ("\nT0: doing reaarnge");
    infodata->prefix_smaller[0] = 0;
    int j=1;
    for (; j<= infodata->nthreads; j++)
      infodata->prefix_smaller[j] = infodata->prefix_smaller[j-1] + infodata->nSmaller[j-1];

    // modify infodata->prefix_larger

    infodata->prefix_larger[0] = 0;
    j=1;
    for (; j<= infodata->nthreads; j++)
      infodata->prefix_larger[j] = infodata->prefix_larger[j-1] + infodata->nGreater[j-1];

    //pthread_mutex_lock (&prefix_mutex);
    //pthread_cond_broadcast(&prefix_cv);
    //pthread_mutex_unlock (&prefix_mutex);
    pthread_barrier_wait (infodata->pbarr);
    printf ("\nT0: @@ bcacst done");
  }
  else {
    printf ("\ntid %d: ---> start waiting for bcast", tid);
    pthread_barrier_wait (infodata->pbarr);
    #if 0
    pthread_mutex_lock (&prefix_mutex);
    pthread_cond_wait(&prefix_cv, &prefix_mutex);
    pthread_mutex_unlock (&prefix_mutex);
    #endif
    printf ("\ntid %d: --> end bcast", tid);
  }
  
  #if 0
  //mylib_barrier(infodata->barr, infodata->nthreads);
  printf ("\ntid %d: --> start 2nd barr ", tid);
  pthread_barrier_wait (infodata->pbarr);
  printf ("\ntid %d: --> END 2nd barr ", tid);
  #endif

  fflush(stdout);

  // all threads can now access global prefix_smaller and prefix_larger
  int offsetSmaller = infodata->prefix_smaller[tid];
  int offsetLarger = infodata->prefix_smaller[infodata->nthreads] + infodata->prefix_larger[tid];

  //rearrange 
  for(i = start; i < end; i++) {
    if (infodata->tarray[i] <= infodata->pivot)
      infodata->array[offsetSmaller++] = infodata->tarray[i];
    else
      infodata->array[offsetLarger++] = infodata->tarray[i];
  }

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
