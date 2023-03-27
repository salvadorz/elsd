#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define from_MS_to_uS(val)    ((val) * 1000u)

// Optional: use these functions to add debug or error prints to your application
#ifdef DEBUG
#define DEBUG_LOG(msg,...)  printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) perror("threading PERROR: " msg)
#else
#define DEBUG_LOG(msg, ...)
#define ERROR_LOG(msg, ...)
#endif

void* threadfunc(void* thread_param)
{
  thread_data_t *thread_data = (thread_data_t *)thread_param;
  usleep(from_MS_to_uS(thread_data->wait_to_get_ms));

  if (0u == pthread_mutex_lock(thread_data->mutex)) {
    usleep(from_MS_to_uS(thread_data->wait_to_rel_ms));

    if (0u == pthread_mutex_unlock(thread_data->mutex))
      thread_data->thread_complete_success = true;
    else
      ERROR_LOG("pthread_mutex_unlock");
  }
  else {
    ERROR_LOG("pthread_mutex_lock");
  }
  
  return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms,
                                  int wait_to_release_ms) {
  bool return_val = false;
  thread_data_t *thread_func_params = malloc(sizeof(thread_data_t));

  if (NULL != thread_func_params) {
    thread_func_params->mutex = mutex;
    thread_func_params->wait_to_get_ms = wait_to_obtain_ms;
    thread_func_params->wait_to_rel_ms = wait_to_release_ms;
    thread_func_params->thread_complete_success = false;

    // Create a thread
    if (0u == pthread_create(thread, NULL, threadfunc, (void *)thread_func_params)) {
      return_val = true;
    }
    
  }

  return return_val;
}

