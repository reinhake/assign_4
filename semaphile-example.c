#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

struct targs {
  sem_t *sem; /* semaphore */
  char *shm; /* shared memory */ 
  size_t shm_sz; /* size of shared memory */
};

void * producer(void *targs){
  srand(time(0));
  sem_t *sem = ((struct targs *) targs)->sem; 

  char *shm = ((struct targs *) targs)->shm;
  size_t shm_sz = ((struct targs *) targs)->shm_sz;
  
  size_t idx = 0;
  while (1) {
    int val;
    do {
      /* Loop while buffer is full */
      sem_getvalue(sem, &val);
    } while (val >= shm_sz);

    /* Get a random character and put in buffer*/
    char c = (rand() % ('~' - '!')) + '!';
    shm[idx] = c;
    idx = idx + 1 < shm_sz ? idx + 1 : 0;

    /* Increment the semaphore */
    sem_post(sem);

    /* Simulate doing other things */
    usleep((rand() % 10)*10000);
  }
  return targs;
}

void * consumer(void *targs){
  sem_t *sem = ((struct targs *) targs)->sem; 
  char *shm = ((struct targs *) targs)->shm;
  size_t shm_sz = ((struct targs *) targs)->shm_sz;
  
  size_t idx = 0;
  while (1) {
    /* wait for data to be available and get it */
    sem_wait(sem);
    char c = shm[idx];
    
    /* Check value of semaphore (remaining data) */
    int val;
    sem_getvalue(sem, &val);

    if (c == 'q') {
      printf("found a q, exiting with %d characters remaining\n", val);
      exit(0);
    }
    else printf("Found: %c, still have: %d characters\n", c, val);

    idx = idx + 1 < shm_sz ? idx + 1 : 0;

    /* simulate doing another task */
    usleep((rand() % 10)*10000);
  }
  return targs;
}


int main(int argc, char *argv[]){
  struct targs targs;
  sem_t sem;
  targs.sem = &sem;
  sem_init(&sem, 0, 0);

  char buf[1024];
  targs.shm = buf;
  targs.shm_sz = sizeof buf;

  pthread_t prod, cons;

  pthread_create(&prod, NULL, &producer, &targs);
  pthread_create(&cons, NULL, &consumer, &targs);
  /* Exit from consumer thread */
  pthread_join(cons, NULL);
}
