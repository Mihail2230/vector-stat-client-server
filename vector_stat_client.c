#include "sockets.h"
#include "errors.h"
#include "functions.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define DEFAULT_SERVER_CONNECTIONS 1;
#define DEFAULT_SERVER_NAME "trabalhoSO_2"
#define DEFAULT_SERVER_HOST "localhost"
#define DEFAUT_SERVER_PORT  5000
#define LOWER_LIMIT       0
#define UPPER_LIMIT     100

typedef struct
{
int *socketfd;
int *values;
int values_sz;
int min;
int max;
} ThreadArgs;

int get_random (int min, int max) 
{
    return rand() % (max - min + 1) + min;
}

void vector_init_rand (int v[], long dim, int min, int max)
{
    for (long i = 0; i < dim; i++) {
        v[i] = get_random(min, max);
    }
}

void  * process_vector_server(void *_args){
	
	ThreadArgs *args = (ThreadArgs *)_args;
	
	write_vector(*args->socketfd,args->values_sz,args->values);
    
    handle_error_system(writen(*args->socketfd, &args->min, sizeof(int)), "Writing min");
    
    handle_error_system(writen(*args->socketfd, &args->max, sizeof(int)), "Writing max");
    
    int count=0;
    int *vector;
    
    vector = read_vector(*args->socketfd,&count);
	
	printf("\ncount = %d",count);
		if(count!=0){
		printf(" VectorSV=[%d",vector[0]);
		for(int i=1; i<count; i++){
			printf(",%d",vector[i]);
			//sleep(0);
		}
		printf("]");
	}
	printf("\n");
	handle_error_system(close(*args->socketfd), "[cli] closing socket to server");
	
	free(vector);
	free(args->values);
	free(args->socketfd);
	
	return NULL;
	
}

int main (int argc, char * argv[])
{
    char *serverEndpoint  = DEFAULT_SERVER_HOST;
    int   serverPort      = DEFAUT_SERVER_PORT;
    char *name 			  = DEFAULT_SERVER_NAME;
    int connections		  = DEFAULT_SERVER_CONNECTIONS;

	if (argc == 2) {
		connections	   = atoi(argv[1]);
	}
	else if(argc == 3) {
		connections	   = atoi(argv[1]);
        serverEndpoint = argv[2];
	}
	else if (argc == 4) {
		connections	   = atoi(argv[1]);
        serverEndpoint = argv[2];
        serverPort     = atoi(argv[3]);
    }
    else if (argc > 4){
		printf("Erro na quantidade de argumentos.\n");
		exit(0);
	}
    
    printf("client connecting to: %s:%d\n", serverEndpoint, serverPort);
    
    int values_sz;
    int *values_tcp;
    int *values_un;
    int min;
    int max;
    pthread_t th_tcp[connections];
    pthread_t th_un[connections];
    ThreadArgs args_tcp[connections];
    ThreadArgs args_un[connections];
    
    for(int i=0;i<connections;i++){
		
		int socketfd_tcp = tcp_socket_client_init(serverEndpoint, serverPort);
		int socketfd_un = un_socket_client_init(name);
		handle_error_system(socketfd_tcp, "[cli] Server socket init");
		handle_error_system(socketfd_un, "[cli] Server socket init");
		
		//Valores aleatorios para o array no tcp
		values_sz = get_random(1,100);
		if(values_sz==1){
			min = 1;
			max = 1;
		}
		else{
			min = get_random(1,values_sz/2);
			if(values_sz%2==0) max = min + get_random(0,values_sz/2);
			else max = min + get_random(0,values_sz/2+1);
		}
		
		values_tcp = malloc(values_sz*sizeof(int));
		vector_init_rand(values_tcp, values_sz, LOWER_LIMIT, UPPER_LIMIT);
		
		int *socket = malloc(sizeof(int));	
		if(socket == NULL){
			perror("Failed to allocate memory");
			exit(1);
		}
		*socket = socketfd_tcp;
		
		args_tcp[i].socketfd = socket;
		args_tcp[i].values_sz = values_sz;
		args_tcp[i].values = values_tcp;
		args_tcp[i].min = min;
		args_tcp[i].max = max;
		
		handle_error_system(pthread_create(&th_tcp[i], NULL, process_vector_server, &args_tcp[i]),"[cli] Creating thread tcp");
		
		//valores aleatorios para o array no unix
		values_sz = get_random(1,100);
		if(values_sz==1){
			min = 1;
			max = 1;
		}
		else{
			min = get_random(1,values_sz/2);
			if(values_sz%2==0) max = min + get_random(0,values_sz/2);
			else max = min + get_random(0,values_sz/2+1);
		}
		
		values_un = malloc(values_sz*sizeof(int));
		vector_init_rand(values_un, values_sz, LOWER_LIMIT, UPPER_LIMIT);
		
		int *socket2 = malloc(sizeof(int));
		if(socket2 == NULL){
			perror("Failed to allocate memory");
			exit(1);
		}	
		*socket2 = socketfd_un;	
		
		args_un[i].socketfd = socket2;
		args_un[i].values_sz = values_sz;
		args_un[i].values = values_un;
		args_un[i].min = min;
		args_un[i].max = max;
		
		handle_error_system(pthread_create(&th_un[i], NULL, process_vector_server, &args_un[i]),"[cli] Creating thread unix");

	}
	
	for(int i=0; i<connections;i++){
		handle_error_system(pthread_join(th_tcp[i], NULL),"[cli] Waiting for thread tcp");
		
		handle_error_system(pthread_join(th_un[i], NULL),"[cli] Waiting for thread unix");
		
	}

    return 0;
}
