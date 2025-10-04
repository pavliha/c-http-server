#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker_thread(void *arg);

thread_pool_t *thread_pool_create(int thread_count) {
  if (thread_count <= 0) {
    thread_count = DEFAULT_THREAD_COUNT;
  }

  thread_pool_t *pool = (thread_pool_t *) malloc(sizeof(thread_pool_t));
  if (!pool) {
    return NULL;
  }

  // Initialize pool
  pool->thread_count = thread_count;
  pool->queue_size   = MAX_QUEUE_SIZE;
  pool->queue_front  = 0;
  pool->queue_rear   = 0;
  pool->queue_count  = 0;
  pool->shutdown     = false;

  // Allocate threads and queue
  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * thread_count);
  pool->queue   = (task_t *) malloc(sizeof(task_t) * MAX_QUEUE_SIZE);

  if (!pool->threads || !pool->queue) {
    free(pool->threads);
    free(pool->queue);
    free(pool);
    return NULL;
  }

  // Initialize mutex and condition variables
  if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0 ||
      pthread_cond_init(&pool->queue_cond, NULL) != 0 ||
      pthread_cond_init(&pool->queue_not_full, NULL) != 0) {
    free(pool->threads);
    free(pool->queue);
    free(pool);
    return NULL;
  }

  // Create worker threads
  for (int i = 0; i < thread_count; i++) {
    if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
      thread_pool_destroy(pool);
      return NULL;
    }
  }

  return pool;
}

bool thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg) {
  if (!pool || !function) {
    return false;
  }

  pthread_mutex_lock(&pool->queue_mutex);

  // Wait if queue is full
  while (pool->queue_count == pool->queue_size && !pool->shutdown) {
    pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
  }

  if (pool->shutdown) {
    pthread_mutex_unlock(&pool->queue_mutex);
    return false;
  }

  // Add task to queue
  pool->queue[pool->queue_rear].function = function;
  pool->queue[pool->queue_rear].arg      = arg;
  pool->queue_rear                       = (pool->queue_rear + 1) % pool->queue_size;
  pool->queue_count++;

  // Signal waiting threads
  pthread_cond_signal(&pool->queue_cond);
  pthread_mutex_unlock(&pool->queue_mutex);

  return true;
}

void thread_pool_destroy(thread_pool_t *pool) {
  if (!pool) {
    return;
  }

  pthread_mutex_lock(&pool->queue_mutex);
  pool->shutdown = true;
  pthread_cond_broadcast(&pool->queue_cond);
  pthread_cond_broadcast(&pool->queue_not_full);
  pthread_mutex_unlock(&pool->queue_mutex);

  // Wait for all threads to finish
  for (int i = 0; i < pool->thread_count; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  // Cleanup
  pthread_mutex_destroy(&pool->queue_mutex);
  pthread_cond_destroy(&pool->queue_cond);
  pthread_cond_destroy(&pool->queue_not_full);

  free(pool->threads);
  free(pool->queue);
  free(pool);
}

int thread_pool_get_active_count(thread_pool_t *pool) {
  if (!pool) {
    return 0;
  }

  pthread_mutex_lock(&pool->queue_mutex);
  int count = pool->queue_count;
  pthread_mutex_unlock(&pool->queue_mutex);

  return count;
}

static void *worker_thread(void *arg) {
  thread_pool_t *pool = (thread_pool_t *) arg;

  while (true) {
    pthread_mutex_lock(&pool->queue_mutex);

    // Wait for tasks or shutdown
    while (pool->queue_count == 0 && !pool->shutdown) {
      pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
    }

    if (pool->shutdown && pool->queue_count == 0) {
      pthread_mutex_unlock(&pool->queue_mutex);
      break;
    }

    // Get task from queue
    task_t task       = pool->queue[pool->queue_front];
    pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
    pool->queue_count--;

    // Signal that queue is not full
    pthread_cond_signal(&pool->queue_not_full);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Execute task
    if (task.function) {
      task.function(task.arg);
    }
  }

  return NULL;
}
