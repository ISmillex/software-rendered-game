#include "strip.h"
#include "raster.h"
#include "display.h"
#include "flags.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    StripPool *pool;
    int        strip_index;
} WorkerArg;

static void *strip_worker_func(void *arg) {
    WorkerArg *wa = (WorkerArg *)arg;
    StripPool *pool = wa->pool;
    int si = wa->strip_index;
    int local_gen = 0;

    while (1) {
        pthread_mutex_lock(&pool->mutex);
        while (pool->generation == local_gen && !pool->shutdown) {
            pthread_cond_wait(&pool->cond_work, &pool->mutex);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }
        local_gen = pool->generation;
        pthread_mutex_unlock(&pool->mutex);

        Strip *strip = &pool->strips[si];
        bool wireframe = g_flags.show_wireframe;
        for (int i = 0; i < strip->bucket_count; i++) {
            const Chunk *chunk = strip->bucket[i];
            if (wireframe) {
                raster_wireframe_triangle(chunk, strip->x_start, strip->x_end);
            } else {
                switch (chunk->type) {
                    case CHUNK_COLORED:
                        raster_colored_triangle(chunk, strip->x_start, strip->x_end);
                        break;
                    case CHUNK_TEXTURED:
                        raster_textured_triangle(chunk, strip->x_start, strip->x_end);
                        break;
                }
            }
        }

        pthread_mutex_lock(&pool->mutex);
        pool->workers_busy--;
        if (pool->workers_busy == 0) {
            pthread_cond_signal(&pool->cond_done);
        }
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

void strip_pool_init(StripPool *pool, int num_strips) {
    pool->strip_count  = num_strips;
    pool->strips       = malloc(num_strips * sizeof(Strip));
    pool->thread_count = num_strips;
    pool->threads      = malloc(num_strips * sizeof(pthread_t));
    pool->shutdown       = false;
    pool->generation     = 0;
    pool->workers_busy   = 0;

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond_work, NULL);
    pthread_cond_init(&pool->cond_done, NULL);

    int strip_width = WINDOW_WIDTH / num_strips;
    for (int i = 0; i < num_strips; i++) {
        pool->strips[i].x_start      = i * strip_width;
        pool->strips[i].x_end        = (i == num_strips - 1) ? WINDOW_WIDTH : (i + 1) * strip_width;
        pool->strips[i].bucket       = malloc(MAX_STRIP_CHUNKS * sizeof(Chunk *));
        pool->strips[i].bucket_count = 0;
    }

    for (int i = 0; i < num_strips; i++) {
        WorkerArg *arg = malloc(sizeof(WorkerArg));
        arg->pool = pool;
        arg->strip_index = i;
        pthread_create(&pool->threads[i], NULL, strip_worker_func, arg);
    }
}

void strip_pool_distribute(StripPool *pool, const Chunk *chunks, int chunk_count) {
    for (int s = 0; s < pool->strip_count; s++) {
        pool->strips[s].bucket_count = 0;
    }

    for (int i = 0; i < chunk_count; i++) {
        ScreenAABB bb = chunk_screen_aabb(&chunks[i]);

        if (bb.x_max < 0 || bb.x_min >= WINDOW_WIDTH ||
            bb.y_max < 0 || bb.y_min >= WINDOW_HEIGHT) {
            continue;
        }

        for (int s = 0; s < pool->strip_count; s++) {
            Strip *strip = &pool->strips[s];
            if (bb.x_min < strip->x_end && bb.x_max > strip->x_start) {
                if (strip->bucket_count < MAX_STRIP_CHUNKS) {
                    strip->bucket[strip->bucket_count++] = &chunks[i];
                }
            }
        }
    }
}

void strip_pool_render(StripPool *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->workers_busy = pool->thread_count;
    pool->generation++;
    pthread_cond_broadcast(&pool->cond_work);
    pthread_mutex_unlock(&pool->mutex);

    pthread_mutex_lock(&pool->mutex);
    while (pool->workers_busy > 0) {
        pthread_cond_wait(&pool->cond_done, &pool->mutex);
    }
    pthread_mutex_unlock(&pool->mutex);
}

void strip_pool_destroy(StripPool *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->cond_work);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    for (int i = 0; i < pool->strip_count; i++) {
        free(pool->strips[i].bucket);
    }
    free(pool->strips);
    free(pool->threads);

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond_work);
    pthread_cond_destroy(&pool->cond_done);
}
