#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "errors.h"

typedef void *(*wi_function_t)(void *);

typedef struct{
	void ** buffer;
	int iPut;
	int iGet;
	int nelems;
	int maxCapacity;
	pthread_cond_t cEsperaEspacoLivre;
	pthread_cond_t cEsperaEspacoOcupado;
	pthread_mutex_t mutex;
}SharedBuffer;

typedef struct {
	SharedBuffer workqueue;
	pthread_t * workers_threads;
	int work;
	int nThreads;
}threadpool_t;

typedef struct{
	wi_function_t func;
	void* args;
}workitem_t;

typedef struct {
	int value;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} countdown_t;

typedef struct{
	int start;
	int end;
	int maximo;
	int minimo;
	int count;
	int *array_v;
	int *array_sv;
	countdown_t *cd;
}ThreadpoolArgs;

int countdown_init (countdown_t *cd, int initialValue);
int countdown_destroy (countdown_t *cd);
int countdown_wait (countdown_t *cd);
int countdown_down (countdown_t *cd);

void sharedBuffer_init (SharedBuffer *sb, int capacity);
void sharedBuffer_destroy (SharedBuffer *sb);
void sharedBuffer_Put (SharedBuffer *sb, void *data);
void * sharedBuffer_Get (SharedBuffer *sb);
void * worker_thread(void * _args);
int threadpool_init (threadpool_t *tp, int queueDim, int nthreads);
int threadpool_submit(threadpool_t *tp, wi_function_t func, void* args);
int threadpool_destroy (threadpool_t *tp);

void * fillArray_2 (void *_args);
int vector_get_in_range_with_thread_pool (int v[], int v_sz, int sv[], int min, int max, threadpool_t *tp);
