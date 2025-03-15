#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "sockets.h"
#include "errors.h"
#include "functions.h"
#include "functions_threadpool.h"

#define DEFAULT_SERVER_NAME "trabalhoSO_2"
#define DEFAUT_SERVER_PORT  5000
#define DEFAULT_SERVER_ARG "-t"

typedef struct{
	int ligacoes;
	int operacoes;
	double total_vz;
	pthread_mutex_t mutex;
}Estatisticas;

typedef struct
{
int socketfd;
char type;
char *arg;
threadpool_t *tp;
threadpool_t *tp_vector;
Estatisticas *e;
} ThreadArgs;

typedef struct
{
int *socketfd;
char *type;
threadpool_t *tp_vector;
Estatisticas *e;
} ThreadArgs2;

void estatisticas_init(Estatisticas *e){
	e->ligacoes = 0;
	e->operacoes = 0;
	e->total_vz = 0;
	pthread_mutex_init(&e->mutex, NULL);
}

void estatisticas_ligacao(Estatisticas *e){
	pthread_mutex_lock(&e->mutex);
	e->ligacoes++;
	pthread_mutex_unlock(&e->mutex);
}

void estatisticas_operacao(Estatisticas *e){
	pthread_mutex_lock(&e->mutex);
	e->operacoes++;
	pthread_mutex_unlock(&e->mutex);
}

void estatisticas_total_vz(Estatisticas *e, double tamanho){
	pthread_mutex_lock(&e->mutex);
	e->total_vz += tamanho;
	pthread_mutex_unlock(&e->mutex);
}

void commands_loop(threadpool_t *tp){
	
	char tecla;
	
	while(1){
		
		tecla = getchar();
		if(tecla == 't' || tecla == 'T'){
			threadpool_destroy(tp);
			printf("Desliga Servidor.\n");
			break;
		}
		
	}
	
}

void * statistics_print(void *_args){
	
	Estatisticas *e = (Estatisticas *) _args;
	
	while(1){
		printf("Ligações: %d | ", e->ligacoes);
		printf("Operações: %d | ", e->operacoes);
		printf("Média: %f\n", e->total_vz/e->ligacoes);
		sleep(1);
	}
	
}

void * handle_client (void *_args)
{
	
	ThreadArgs2 *args = (ThreadArgs2 *)_args;
	int *socketfd = args->socketfd;
	char *tipo = args->type;
	threadpool_t *tp_vector = args->tp_vector;
	Estatisticas *e = args->e;
	int vector_sz = 0;
	int *vector;
	
	vector = read_vector(*socketfd, &vector_sz);
	
	estatisticas_total_vz(e, vector_sz);
	
	int min;
	int max;
	
	handle_error_system(readn(*socketfd, &min, sizeof(int)), "[srv] Reading min");
	handle_error_system(readn(*socketfd, &max, sizeof(int)), "[srv] Reading max");
	
	int *sv = malloc(vector_sz * sizeof(int));
	int count=0;
	
	//if para verificar se processamos o vetor com threads ou processos.
    
    if(strcmp(tipo,"-t")==0){
		count = vector_get_in_range_with_thread_pool(vector, vector_sz, sv, min, max, tp_vector);
		printf("size: %d | min= %d | max= %d | count= %d | Type = Threads\n",vector_sz,min,max,count);
	}
    else{
		count = vector_get_in_range_with_processes(vector, vector_sz, sv, min, max, 2);
		printf("size: %d | min= %d | max= %d | count= %d | Type = Processes\n",vector_sz,min,max,count);
	}
    
    
    write_vector(*socketfd, count, sv);
    
    handle_error_system(close(*socketfd), "[srv] closing socket to client");
    
    free(vector);
    free(sv);
    free(socketfd);
    free(args);
    
    estatisticas_operacao(e);
    
    return NULL;
}

void * connection_loop (void *_args){
	
	//pthread_t th_client;
	
	ThreadArgs *args = (ThreadArgs *)_args;
	
	int socket = args->socketfd;
	char tipo = args->type;
	char *thopr = args->arg;
	threadpool_t *tp = args->tp;
	threadpool_t *tp_vector = args->tp_vector;
	Estatisticas *e = args->e;
	int newsocketfd;

	while ( 1 ) {
		if(tipo == 't') newsocketfd = tcp_socket_server_accept(socket);
		else newsocketfd = un_socket_server_accept(socket);
		
		handle_error_system(newsocketfd, "[srv] Accept new connection");
		
		ThreadArgs2 *argumentos = malloc(sizeof(ThreadArgs2));
		if(argumentos == NULL){
			perror("Failed to allocate memory");
			exit(1);
		}
		
		//variavel dinamica
		int *socket2 = malloc(sizeof(int));
		if (socket2 == NULL) {
            perror("Failed to allocate memory");
            free(argumentos);
            exit(1);
        }	
		*socket2 = newsocketfd;	
		
		argumentos->socketfd = socket2;
		argumentos->type = thopr;
		argumentos->tp_vector = tp_vector;
		argumentos->e = e;
		
		//pthread_create(&th_client, NULL, handle_client, argumentos);
		//pthread_detach(th_client);
		
		threadpool_submit(tp,(wi_function_t) handle_client, argumentos);
		estatisticas_ligacao(e);
	}
	
}

int main (int argc, char * argv[])
{
    int serverPort = DEFAUT_SERVER_PORT;
	char *name = DEFAULT_SERVER_NAME;
	char *arg  = DEFAULT_SERVER_ARG;


	if (argc == 2) {
		arg = argv[1];
    }
    else if (argc == 3) {
		arg = argv[1];
        serverPort = atoi(argv[2]);
    }
    else if(argc > 3) {
		printf("Erro na quantidade de argumentos.");
		exit(0);
	}

    if ( serverPort < 1024 ) {
        printf("Port sould be above 1024\n");
        exit(EXIT_FAILURE);		 
    }
    
    if(strcmp(arg,"-t")!=0 && strcmp(arg,"-p")!=0){
		printf("Erro no argumento do tipo de processamento do vetor\n");
		exit(0);
	}
           	
    printf("Server using port: %d\n", serverPort);

    int socketfd_tcp = tcp_socket_server_init(serverPort);
    
    unlink(name);
    int socketfd_un = un_socket_server_init(name);

    handle_error_system(socketfd_tcp, "[srv] Server socket init tcp");
    handle_error_system(socketfd_un, "[srv] Server socket init unix");
    
    pthread_t th_socket_tcp;
    pthread_t th_socket_un;
    pthread_t printStatistics;
    
    threadpool_t tp;
    threadpool_init(&tp, 10, 4);
    
    threadpool_t tp_vector;
    threadpool_init(&tp_vector, 20, 10);
    
    Estatisticas e;
    estatisticas_init(&e);
    
    ThreadArgs args_tcp;
    ThreadArgs args_un;
    
    args_tcp.socketfd = socketfd_tcp;
    args_tcp.type = 't';
    args_tcp.arg = arg;
    args_tcp.tp = &tp;
    args_tcp.tp_vector = &tp_vector;
    args_tcp.e = &e;
    
    args_un.socketfd = socketfd_un;
    args_un.type = 'u';
    args_un.arg = arg;
    args_un.tp = &tp;
    args_un.tp_vector = &tp_vector;
    args_un.e = &e;
    
    pthread_create(&th_socket_tcp, NULL, connection_loop, &args_tcp);
    pthread_create(&th_socket_un, NULL, connection_loop, &args_un);
    pthread_create(&printStatistics, NULL, statistics_print, &e);
    
	commands_loop(&tp);

    return 0;
}
