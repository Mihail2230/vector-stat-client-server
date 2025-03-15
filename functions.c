#include <stdio.h>
#include "functions.h"

typedef struct
{
int start;
int end;
int maximo;
int minimo;
int count;
int *array_v;
int *array_sv;
} ThreadArgs;


void * fillArray (void *_args){
	
	ThreadArgs *args = (ThreadArgs *)_args;
	
	args->count = args->start;
	
	for (int i = args->start; i < args->end; i++) {
		if (args->array_v[i] >= args->minimo && args->array_v[i] <= args->maximo) {
			args->array_sv[args->count++] = args->array_v[i];
		}
	}
	
	args->count = args->count - args->start;
	

	return NULL;
	
}

int vector_get_in_range_with_threads (int v[], int v_sz, int sv[], int min, int max, int nThreads)
{
	
	int dimPart = (max-min)/nThreads;
	//int dimPart = v_sz/nThreads;
	int contador = 0;
	if(v_sz%nThreads != 0){
		//printf("nao divisivel\n");
		dimPart++;
	}
		
	pthread_t th[nThreads];
	ThreadArgs args[nThreads];
	
	for(int i=0;i<nThreads;i++){
		
		args[i].start = i * dimPart + min;
		args[i].end = args[i].start + dimPart;
		if(args[i].end > max) args[i].end = max;
		args[i].maximo = 100;
		args[i].minimo = 50;
		args[i].array_v = v;
		args[i].array_sv = sv;
		
		int error = pthread_create(&th[i], NULL, fillArray, &args[i]);
		if(error!=0){
			errno = error;
			perror("Erro");
			exit(1);
		}
	}
	
	for(int i=0;i<nThreads;i++){
		int error = pthread_join(th[i], NULL);
		if(error!=0){
			errno = error;
			perror("Erro");
			exit(1);
		}
		//if(i!=0){
		for(int j=0;j<args[i].count;j++){
			sv[contador+j]=sv[args[i].start+j];
		}
		//}
		contador += args[i].count;
	 }
	
	return contador;
	
}

int vector_get_in_range_with_processes (int v[], int v_sz, int sv[], int min, int max, int nProcesses)
{
	int pipefd[nProcesses][2];
	int dimPart = (max-min)/nProcesses;
	if(v_sz%nProcesses != 0){
		//printf("nao divisivel\n");
		dimPart++;
	}
		
	pid_t retfork[nProcesses];
	
	for(int i=0;i<nProcesses;i++){
		pipe(pipefd[i]);
		retfork[i]=fork();
		
		if(retfork[i]<0){
			perror("Erro no fork");
			exit(-1);
		}
		else if(retfork[i]==0){
			close(pipefd[i][0]);
			int start = i * dimPart + min;
			int end = start + dimPart;
			if(end > max) end = max;
			int *array = malloc(sizeof(int)*dimPart);
			if (array == NULL) {
				fprintf(stderr, "Erro malloc\n");
				exit(0);
			}
			int count = 0;
			for (long j = start; j < end; j++) {
				if (v[j] >= 50 && v[j] <= 100) {
					array[count++] = v[j];
					//printf("--Valor: %d , Count:%d \n",array[count-1],count);
				}
			}
			
			write(pipefd[i][1],array,sizeof(int)*count);
			close(pipefd[i][1]);
			free(array);
			exit(0);
		}
		
		close(pipefd[i][1]);
	}
    
	int curr = 0;
     
	for(int i=0;i<nProcesses;i++){
		
		int size = sizeof(int)*dimPart;
		int nbytes_rd;
		while( (nbytes_rd = read(pipefd[i][0], &sv[curr],size)) > 0) {
			curr += nbytes_rd/sizeof(int);
			size -= nbytes_rd;
			//printf("%d\n",nbytes_rd/sizeof(int));
		}
		
		if (nbytes_rd) {
			perror("Pai read");
			printf("%d; %ld \n", curr, curr/sizeof(int)); 
			exit(1);
		}
		
		waitpid(retfork[i],NULL,0);
		
		close(pipefd[i][0]);
	}
    
	return curr;
	
}
int * read_vector(int socket, int *vector_sz){
	
	handle_error_system(readn(socket, vector_sz, sizeof(int)), "[srv] Reading vector size");
	
	int *vector = malloc(*vector_sz*sizeof(int));
	
	handle_error_system(readn(socket, vector, *vector_sz * sizeof(int)), "[srv] Reading vector");
	
	return vector;
	
}

void write_vector(int socket, int count, int sv[]){
	
	handle_error_system(writen(socket, &count, sizeof(int)), "[srv] Writing count");
    
    handle_error_system(writen(socket, sv, count*sizeof(int)), "[srv] Writing vector");
}

