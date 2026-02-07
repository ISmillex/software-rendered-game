#ifndef STRIP_H
#define STRIP_H

#include "chunk.h"
#include <pthread.h>
#include <stdbool.h>

#define MAX_STRIP_CHUNKS 8192

typedef struct {
    int           x_start;
    int           x_end;
    const Chunk **bucket;
    int           bucket_count;
} Strip;

typedef struct {
    pthread_t      *threads;
    int             thread_count;
    Strip          *strips;
    int             strip_count;

    pthread_mutex_t mutex;
    pthread_cond_t  cond_work;
    pthread_cond_t  cond_done;
    int             workers_busy;
    int             generation;
    bool            shutdown;
} StripPool;

void strip_pool_init(StripPool *pool, int num_strips);
void strip_pool_distribute(StripPool *pool, const Chunk *chunks, int chunk_count);
void strip_pool_render(StripPool *pool);
void strip_pool_destroy(StripPool *pool);

#endif // STRIP_H
