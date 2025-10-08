/*
Thread pool
A: Stéphane MEYER (Teegre)
C: 2025-10-01
M: 2025-10-01
*/

#include <pthread.h>

#include "pool.h"

#define MAX_THREADS 8
#define QUEUE_SIZE 64

atomic_int q_running = 1;

int conn_queue[QUEUE_SIZE];
int q_head = 0, q_tail = 0, q_count = 0;
pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;

pthread_t workers[MAX_THREADS];

void queue_push(int conn) {
    pthread_mutex_lock(&q_mutex);
    while (q_count == QUEUE_SIZE) {
        pthread_cond_wait(&q_cond, &q_mutex);
    }
    conn_queue[q_tail] = conn;
    q_tail = (q_tail + 1) % QUEUE_SIZE;
    q_count++;
    pthread_cond_broadcast(&q_cond);
    pthread_mutex_unlock(&q_mutex);
}

int queue_pop() {
    pthread_mutex_lock(&q_mutex);
    while(q_count == 0 && q_running) {
        pthread_cond_wait(&q_cond, &q_mutex);
    }
    if (!q_running) {
        if (q_count == 0) {
            pthread_mutex_unlock(&q_mutex);
            return -1;
        }
    }
    int conn = conn_queue[q_head];
    q_head = (q_head + 1) % QUEUE_SIZE;
    q_count--;
    pthread_cond_broadcast(&q_cond);
    pthread_mutex_unlock(&q_mutex);
    return conn;
}

void queue_cleanup() {
    pthread_mutex_lock(&q_mutex);
    q_running = 0;
    pthread_cond_broadcast(&q_cond);
    pthread_mutex_unlock(&q_mutex);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }
}
