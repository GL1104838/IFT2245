#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>

extern bool accepting_connections;

typedef struct server_thread server_thread;
struct server_thread
{
  unsigned int id;
  pthread_t pt_tid;
  pthread_attr_t pt_attr;
};

struct bankersArray
{
	int * allocatedResources;
	int * maxResources;
	bool visited;
};

void st_open_socket (int port_number);
void st_init (int);
void st_process_request (server_thread *, int);
bool st_signal (FILE *, server_thread *); //changed st_signal
void *st_code (void *);
//void st_create_and_start(st);
void st_print_results (FILE *, bool);
#endif
