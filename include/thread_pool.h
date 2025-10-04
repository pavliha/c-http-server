#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_QUEUE_SIZE 1024
#define DEFAULT_THREAD_COUNT 4

// Task structure for work queue
typedef struct {
  void (*function)(void *arg);
  void *arg;
} task_t;

// Thread pool structure
typedef struct {
  pthread_t *threads;
  int thread_count;

  task_t *queue;
  int queue_size;
  int queue_front;
  int queue_rear;
  int queue_count;

  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;
  pthread_cond_t queue_not_full;

  bool shutdown;
} thread_pool_t;

// Thread pool functions
thread_pool_t *thread_pool_create(int thread_count);
bool thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg);
void thread_pool_destroy(thread_pool_t *pool);
int thread_pool_get_active_count(thread_pool_t *pool);

#endif // THREAD_POOL_H
