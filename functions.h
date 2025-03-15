#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sockets.h"
#include "errors.h"

void * fillArray (void *_args);
int vector_get_in_range_with_threads(int v[], int v_sz, int sv[], int min, int max, int nThreads);
int vector_get_in_range_with_processes (int v[], int v_sz, int sv[], int min, int max, int nProcesses);
int * read_vector(int socket, int *vector_sz);
void write_vector(int socket, int count, int sv[]);
