#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "header.h"

int cmpint1(const void *p1, const void *p2) {
return *((int*)p1) - *((int*)p2);
}

void validate(int* output, int num_elements) {
  int i = 0;
  assert(output != NULL);
  for(i = 0; i < num_elements - 1; i++) {
    if (output[i] > output[i+1]) {
      printf("************* NOT sorted *************\n");
      return;
    }
  }
  printf("============= SORTED ===========\n"); 
}

int main(int argc, char **argv)
{
  FILE* fin = NULL;
  int* input = NULL;
  int* c_input = NULL;    //for serail
  int* output = NULL;
  int num_elements, num_threads, i = 0;

  if(argc != 2)
  {printf("Usage: ./pqsort <num of threads>\n");}

  num_threads = atoi(argv[1]);

  //read input_size and input
  if((fin = fopen("input.txt", "r")) == NULL)
  {printf("Error opening input file\n"); exit(0);}

  fscanf(fin, "%d", &num_elements);
  if( !(input = (int *)calloc(num_elements, sizeof(int))) )
  {printf("Memory error\n"); exit(0);}

  if( !(c_input = (int *)calloc(num_elements, sizeof(int))) )
  {printf("Memory error\n"); exit(0);}

  for(i = 0; i < num_elements || feof(fin); i++) {
    fscanf(fin, "%d", &input[i]);
    fscanf(fin, "%d", &c_input[i]);
    }

  if(i < num_elements)
  {printf("Invalid input\n"); exit(0);}

  // Sort time
  printf ("\ncall pqsort internal");
  struct timeval tim;
  gettimeofday(&tim,NULL);
  double start = tim.tv_sec + (tim.tv_usec/1000000.0);

  output = pqsort(input, num_elements, num_threads);

  gettimeofday(&tim,NULL);
  double end = tim.tv_sec + (tim.tv_usec/1000000.0);
  printf("Time taken: %f\n\n", end - start);
  // Sort time

  validate(output, num_elements);

  gettimeofday(&tim,NULL);
  start = tim.tv_sec + (tim.tv_usec/1000000.0);

  // call serial sort
  qsort(c_input, num_elements, sizeof(int), cmpint2);

  end = tim.tv_sec + (tim.tv_usec/1000000.0);
  printf("Time taken (serial): %f\n\n", end - start);

  return 0;
}
