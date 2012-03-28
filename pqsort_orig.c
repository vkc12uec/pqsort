//#include <stdio.h>
#include <pthread.h>

int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

int *pqsort(int* input, int num_elements, int num_threads)
{
  // YOUR CODE GOES HERE
  return input; //return appropriately
}
