// PPLS Exercise 1 Starter File
//
// See the exercise sheet for details
//
// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

/**
 The main strategy of this program is to divide the input data array into equal-sized chunks, 
with each thread operating on one of these chunks. There are three phases of computation, 
with a barrier synchronization in between each phase. 
It is noticeable that the number of items should be larger than the number of threads. 

In terms of the size of each chunk, I take the floor function of NITEMS divided by NTHREADS. 
Surplus items are assigned to the final chunk. 
In terms of the thread creation, firstly, a total of NTHREADS threads are created using POSIX threads. 
Then, for each thread, an argument structure (arg_pack) is allocated and populated with the necessary data: 
a reference to the data array, the thread ID, and the start and end indices for the data array. 
The Worker function is then called for each thread. 

In addition, Barrier synchronization is used to ensure that all threads have completed their work 
before moving to the next phase. A mutex semaphore (barrier) and a condition variable (go) are used to implement the barrier. 
The Barrier function is called at the end of phase 1 and phase 2 to ensure that all threads have completed their work 
before moving on to the next phase. I also used  pthread_join() function to wait for all threads to terminate
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> // in case you want semaphores, but you don't really
                       // need them for this exercise


// Print a helpful message followed by the contents of an array
// Controlled by the value of SHOWDATA, which should be defined
// at compile time. Useful for debugging.
void showdata (char *message,  int *data,  int n) {
  int i; 

if (SHOWDATA) {
    printf ("%s", message);
    for (i=0; i<n; i++ ){
     printf (" %d", data[i]);
    }
    printf("\n");
  }
}

// Check that the contents of two integer arrays of the same length are equal
// and return a C-style boolean
int checkresult (int* correctresult,  int *data,  int n) {
  int i; 

  for (i=0; i<n; i++ ){
    if (data[i] != correctresult[i]) return 0;
  }
  return 1;
}

// Compute the prefix sum of an array **in place** sequentially
void sequentialprefixsum (int *data, int n) {
  int i;

  for (i=1; i<n; i++ ) {
    data[i] = data[i] + data[i-1];
  }
}


// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
pthread_mutex_t barrier; // mutex semaphore for the barrier
pthread_cond_t go; // condition variable for leaving
int numArrived = 0;

typedef struct arg_pack{
  int * data;
  int id;
  int start;
  int end;
}arg_pack;

void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == NTHREADS) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else {
    pthread_cond_wait(&go, &barrier);
  }
  pthread_mutex_unlock(&barrier);
}

void *Worker(void *data){
  arg_pack *threadArgs = (arg_pack *)data;

  //PHASE 1
  for (int i = threadArgs->start; i < threadArgs->end; i++){
    threadArgs->data[i+1] = threadArgs->data[i+1] + threadArgs->data[i];
  }
  Barrier();
  //PHASE 2
  if (threadArgs->id == 0){
    for (int i = threadArgs->end; i < (NITEMS/NTHREADS)* (NTHREADS-2); i = i + (NITEMS/NTHREADS)){
      threadArgs->data[i+(NITEMS/NTHREADS)] = threadArgs->data[i+(NITEMS/NTHREADS)] + threadArgs->data[i];
    }
    if (NTHREADS > 1){
      threadArgs->data[NITEMS-1] = threadArgs->data[NITEMS-1] + threadArgs->data[NITEMS-1 - (NITEMS/NTHREADS)-(NITEMS%NTHREADS)];
    }

  }
  Barrier();

  //PHASE 3
  if (threadArgs->id != 0){
    for (int i = threadArgs->start; i < threadArgs->end; i++){
      threadArgs->data[i] = threadArgs->data[i] + threadArgs->data[threadArgs->start-1];
    }
  }
}

void parallelprefixsum (int *data, int n) {
  if (NTHREADS < 1){
    printf("You need to have at least one thread!!");
    exit(EXIT_FAILURE);
  }
  if (NITEMS < 1){
    printf("You need to have at least one item in the array!!");
    exit(EXIT_FAILURE);
  }
  if (NTHREADS > NITEMS){
    printf("The item number should be larger than thread number");
    exit(EXIT_FAILURE);
  }

  pthread_t thread[NTHREADS];

  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  int status;
  
  for (int i = 0; i < NTHREADS; i++){
    arg_pack *d = (arg_pack *) malloc(sizeof(arg_pack));
    d->id = i;
    d->data = data;
    if (i < NTHREADS-1){
      int start = i * (NITEMS/NTHREADS);
      int end = (i+1) * (NITEMS/NTHREADS) - 1;
      d->start = start;
      d->end = end;
    } else{
      int start = i * (NITEMS/NTHREADS);
      int end = NITEMS - 1;
      d->start = start;
      d->end = end;
    }
    status = pthread_create(&thread[i], NULL, Worker, (void *)d);
    if (status == -1){
      printf("The thread create not successfully, Please check!!");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < NTHREADS; i++){
    pthread_join(thread[i], NULL);
  }
}


int main (int argc, char* argv[]) {

  int *arr1, *arr2, i;

  // Check that the compile time constants are sensible
  if ((NITEMS>10000000) || (NTHREADS>32)) {
    printf ("So much data or so many threads may not be a good idea! .... exiting\n");
    exit(EXIT_FAILURE);
  }

  // Create two copies of some random data
  arr1 = (int *) malloc(NITEMS*sizeof(int));
  arr2 = (int *) malloc(NITEMS*sizeof(int));
  srand((int)time(NULL));
  for (i=0; i<NITEMS; i++) {
     arr1[i] = arr2[i] = rand()%5;
  }
  showdata ("initial data          : ", arr1, NITEMS);

  // Calculate prefix sum sequentially, to check against later on
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);

  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}
