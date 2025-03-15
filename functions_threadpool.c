#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <semaphore.h>
#include "functions_threadpool.h"
#include "error.h"

int countdown_init (countdown_t *cd, int initialValue){
	
	if(initialValue<1) return -1;
	
	cd->value = initialValue;
	pthread_mutex_init(&cd->mutex,NULL);
	pthread_cond_init(&cd->cond, NULL);
	
	return 1;
}

int countdown_destroy (countdown_t *cd){
	
	if(pthread_mutex_destroy(&cd->mutex)) return -1;
	if(pthread_cond_destroy(&cd->cond)) return -1;
	
	return 1;
	
}

int countdown_wait (countdown_t *cd){
	
	pthread_mutex_lock(&cd->mutex);
	
	if(cd->value<0){
		pthread_mutex_unlock(&cd->mutex);
		return -1;
	}
	
	while(cd->value>0){
		pthread_cond_wait(&cd->cond,&cd->mutex);
	}
	
	pthread_mutex_unlock(&cd->mutex);
	
	return 1;
	
}

int countdown_down (countdown_t *cd){
	
	pthread_mutex_lock(&cd->mutex); 
	
	if(cd->value<1){
		pthread_mutex_unlock(&cd->mutex);
		return -1;
	}
	else{
		cd->value--;
		if(cd->value == 0){
			pthread_cond_broadcast(&cd->cond);
		}
	}
	
	pthread_mutex_unlock(&cd->mutex);
	
	return 1;
}

void sharedBuffer_init (SharedBuffer *sb, int capacity){
	sb->buffer = (void **)malloc(capacity * sizeof(void *));
	sb->iGet = 0;
	sb->iPut = 0;
	sb->nelems = 0;
	sb->maxCapacity = capacity;pthread_cond_init(&sb->cEsperaEspacoLivre, NULL);
	pthread_cond_init(&sb->cEsperaEspacoOcupado, NULL);
	pthread_mutex_init(&sb->mutex, NULL);
}

void sharedBuffer_destroy (SharedBuffer *sb){
	free(sb->buffer);
	pthread_cond_destroy(&sb->cEsperaEspacoLivre);
	pthread_cond_destroy(&sb->cEsperaEspacoOcupado);
	pthread_mutex_destroy(&sb->mutex);
}

void sharedBuffer_Put (SharedBuffer *sb, void *data){
	pthread_mutex_lock(&sb->mutex);
	while (sb->nelems == sb->maxCapacity) {
		pthread_cond_wait(&sb->cEsperaEspacoLivre, &sb->mutex);
	}
	sb->buffer[sb->iPut] = data;
	sb->iPut = (sb->iPut+1)%sb->maxCapacity;
	++sb->nelems;
	pthread_cond_signal(&sb->cEsperaEspacoOcupado);
	pthread_mutex_unlock(&sb->mutex);
}

void * sharedBuffer_Get (SharedBuffer *sb){
	void *ret;
	pthread_mutex_lock(&sb->mutex);
	while (sb->nelems == 0) {
		pthread_cond_wait(&sb->cEsperaEspacoOcupado,&sb->mutex);
	}
	ret=sb->buffer[sb->iGet];
	sb->iGet = (sb->iGet+1)%sb->maxCapacity;
	--sb->nelems;
	pthread_cond_signal(&sb->cEsperaEspacoLivre);
	pthread_mutex_unlock(&sb->mutex);
	return ret;
}

void * worker_thread(void * _args){
	
	threadpool_t * tp = (threadpool_t *)_args;
	
	while(1){
		workitem_t* wi = sharedBuffer_Get(&tp->workqueue);
		if(wi == NULL) break;
		wi->func(wi->args);
	}
	
	return NULL;
}

int threadpool_init (threadpool_t *tp, int queueDim, int nthreads){
	
	sharedBuffer_init(&tp->workqueue, queueDim);
	tp-> workers_threads = malloc(sizeof(pthread_t)*nthreads);
	if(tp->workers_threads == NULL) return -1;
	tp->work = 1;
	tp->nThreads = nthreads;
	
	for(int i=0; i<nthreads; i++){
		if(pthread_create(&tp->workers_threads[i], NULL, worker_thread, tp)) return -1;
	}
	
	return 1;
	
}

int threadpool_submit(threadpool_t *tp, wi_function_t func, void* args){
	
	if(tp->work == 0 && func!=NULL && args!=NULL) return -1;
	
	workitem_t *wi = malloc(sizeof(workitem_t));
	if(wi == NULL) return -1;
	
	wi->func = func;
	wi->args = args;
	
	if(func==NULL && args==NULL) wi = NULL;
	
	sharedBuffer_Put(&tp->workqueue, wi);
	
	return 1;
}

int threadpool_destroy (threadpool_t *tp){
	
	tp->work = 0;
	
    for (int i = 0; i < tp->nThreads; i++) {
        handle_error_system(threadpool_submit(tp, NULL, NULL), "Erro na threadpool_submit.");
    }

    for (int i = 0; i < tp->nThreads; i++) {
        if(pthread_join(tp->workers_threads[i], NULL)) return -1;
    }
	
	sharedBuffer_destroy(&tp->workqueue);
	free(tp->workers_threads);
	
	return 1;
}

void * fillArray_2 (void *_args){
	
	ThreadpoolArgs *args = (ThreadpoolArgs*)_args;
	
	args->count = args->start;
	
	for (int i = args->start; i < args->end; i++) {
		if (args->array_v[i] >= args->minimo && args->array_v[i] <= args->maximo) {
			args->array_sv[args->count++] = args->array_v[i];
		}
	}
	
	args->count = args->count - args->start;
	
	handle_error_system(countdown_down(args->cd), "Erro no countdown_down");

	return NULL;
	
}

int vector_get_in_range_with_thread_pool (int v[], int v_sz, int sv[], int min, int max, threadpool_t *tp){
	
	
	int dimPart = v_sz/4;
	int contador = 0;
	if(v_sz%4 != 0){
		dimPart++;
	}
	
	countdown_t *cd = malloc(sizeof(countdown_t));
	ThreadpoolArgs args[4];
	
	handle_error_system(countdown_init(cd, 4), "Erro no countdown_init.");
	
	for(int i=0;i<4;i++){
		
		args[i].start = i * dimPart;
		args[i].end = args[i].start + dimPart;
		if(args[i].end > v_sz) args[i].end = v_sz;
		args[i].maximo = max;
		args[i].minimo = min;
		args[i].array_v = v;
		args[i].array_sv = sv;
		args[i].cd = cd;
		
		handle_error_system(threadpool_submit(tp, (wi_function_t) fillArray_2, &args[i]), "ThreadPool fechada.");
	}
	
	handle_error_system(countdown_wait(cd), "Erro no countdown_wait");
	
	for(int i=0;i<4;i++){
		
		if(i!=0){
			for(int j=0;j<args[i].count;j++){
				sv[contador+j]=sv[args[i].start+j];
			}
		}
		contador +=args[i].count;
	}
	
	handle_error_system(countdown_destroy(cd), "Erro no countdown_destroy.");
	
	return contador;
}
