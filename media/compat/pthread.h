#pragma once
typedef int pthread_mutex_t;
static inline int pthread_mutex_init(pthread_mutex_t*p,void*a){*p=0;return 0;}
static inline int pthread_mutex_lock(pthread_mutex_t*p){return 0;}
static inline int pthread_mutex_unlock(pthread_mutex_t*p){return 0;}
static inline int pthread_mutex_destroy(pthread_mutex_t*p){return 0;}
