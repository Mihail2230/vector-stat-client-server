CC = gcc
CFLAGS = -g -Wall

BINS = vector_stat_server vector_stat_client

all: $(BINS)

vector_stat_server: vector_stat_server.o sockets.o errors.o functions.o functions_threadpool.o

vector_stat_server.o: vector_stat_server.c sockets.h errors.h functions.h functions_threadpool.h


vector_stat_client: vector_stat_client.o sockets.o errors.o functions.o functions_threadpool.o

vector_stat_client.o: vector_stat_client.c sockets.h errors.h functions.h functions_threadpool.h



sockets.o: sockets.c sockets.h

errors.o: errors.c errors.h

functions.o: functions.c functions.h sockets.h errors.h

functions_threadpool.o: functions_threadpool.c functions_threadpool.h errors.h

clean:
	$(RM) *.o $(BINS)
