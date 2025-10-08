#ifndef __POOL_H__
#define __POOL_H__

#include <pthread.h>
#include <stdatomic.h>

#define MAX_THREADS 8
#define QUEUE_SIZE 64

extern atomic_int q_running;
extern pthread_t workers[MAX_THREADS];

void queue_push(int);
int queue_pop(void);
void queue_cleanup(void);

#endif /* ifndef __POOL_H__ */
