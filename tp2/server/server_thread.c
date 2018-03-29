#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "server_thread.h"

//Added includes
#include "../protocol/communication.h"
#include <semaphore.h>

#include <netinet/in.h>
#include <netdb.h>

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <signal.h>

#include <time.h>

enum { NUL = '\0' };

enum {
  /* Configuration constants.  */
  max_wait_time = 30,
  server_backlog_size = 5
};

unsigned int server_socket_fd;

// Nombre de client enregistré.
int nb_registered_clients;

// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

//Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Nombre de threads Server
int nbServerThreads = 0;

//Banker's Algorithm data
int * bankersAlgoAvailable;
int * bankersAlgoMaxResources;
unsigned int bankersAlgoNbResources;
struct bankersArray * bankersArray;

static void sigint_handler(int signum) {
  // Code terminaison.
  accepting_connections = 0;
}

void
st_init(int threadsCount)
{
  // Handle interrupt
  signal(SIGINT, &sigint_handler);

  accepting_connections = false;

  // Initialise le nombre de clients connecté.
  nb_registered_clients = 0;

  nbServerThreads = threadsCount;
  bankersArray = malloc(threadsCount * sizeof(struct bankersArray));
  if (bankersArray == NULL) exit(-1);
  // Attend la connection d'un client et initialise les structures pour
  // l'algorithme du banquier.
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int socket = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
  struct communication_data reading_data;
  struct communication_data writing_data;

  while (socket < 0) {
    socket = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
  }

  FILE * socket_r = fdopen(socket, "r");
  FILE * socket_w = fdopen(socket, "w");

  //Set ClientID to -1
  for (int i = 0; i < nbServerThreads; i++) {
    bankersArray[i].clientID = -1;
  }
	//BEG
  do {
    read_communication(socket_r, socket_w, &reading_data);
    if(reading_data.args_count != 1 || reading_data.args[0] < 1) {
      //Invalid Argument
      free(reading_data.args);
      reading_data.args = NULL;
      reading_data.args_count = 0;
      reading_data.communication_type = ERR;
      write_errorMessage(socket_w, "Must BEG for at least one resource");
      printf("Erreur onServerInit BEG\n\n");
      exit(-1);
    }
  } while (reading_data.communication_type != BEG);
  printf("\n\n---BEG---\n\n");
  writing_data.communication_type = ACK;
  writing_data.args = NULL;
  writing_data.args_count = 0;
  write_communication(socket_w, &writing_data);
  bankersAlgoNbResources = reading_data.args[0];
  free(reading_data.args);
  reading_data.args = NULL;
  free(writing_data.args);
  reading_data.args = NULL;
	
  //PRO
  do {
    read_communication(socket_r, socket_w, &reading_data);
    printf("\n\n---PRO---\n\n");
    bankersAlgoAvailable = calloc(bankersAlgoNbResources, sizeof(int));
    if (bankersAlgoAvailable == NULL) exit(-1);
    bankersAlgoMaxResources = calloc(bankersAlgoNbResources, sizeof(int));
    if (bankersAlgoMaxResources == NULL) exit(-1);
    for (int i = 0; i < reading_data.args_count; i++) {
      if (reading_data.args[i] < 1) {
        //Invalid Argument
        free(bankersAlgoAvailable);
        free(bankersAlgoMaxResources);
			  free(reading_data.args);
			  bankersAlgoAvailable = NULL;
			  bankersAlgoMaxResources = NULL;
			  reading_data.args = NULL;
			  reading_data.args_count = 0;
			  reading_data.communication_type = ERR;
			  write_errorMessage(socket_w, 
			  "Must provide at least one of each resource");
			  printf("Erreur onServerInit PRO\n\n");
			  exit(-1);
		  }
		  printf(" %d", reading_data.args[i]);
		  bankersAlgoMaxResources[i] = reading_data.args[i];
		  bankersAlgoAvailable[i] = reading_data.args[i];
	  }
	  printf("\n\n");
	} while (reading_data.communication_type != PRO);

	writing_data.communication_type = ACK;
	writing_data.args = NULL;
	writing_data.args_count = 0;
	write_communication(socket_w, &writing_data);
	
	free(reading_data.args);
	reading_data.args = NULL;
	free(writing_data.args);
	reading_data.args = NULL;

  accepting_connections = true;

	fclose(socket_r);
	fclose(socket_w);
	close(socket);
}

void
st_process_requests (server_thread * st, int socket_fd)
{

  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  bool isInitialize = false;
  bool looping = true;
  struct communication_data reading_data;
  struct communication_data writing_data;
  
  reading_data.args = NULL;
	writing_data.args = NULL;

  if (bankersAlgoNbResources < 1 || 
      bankersAlgoMaxResources == NULL ||
	    bankersAlgoAvailable == NULL) {
	  //ERROR onServerStart
    write_errorMessage(socket_w,"Server not initialized yet!");
	  looping = false;
  }
  else {
		bankersArray[st->id].allocatedResources = 
		  calloc(bankersAlgoNbResources, sizeof(int));
  	if (bankersArray[st->id].allocatedResources == NULL) exit(-1);
  	bankersArray[st->id].maxResources = 
		  calloc(bankersAlgoNbResources, sizeof(int));
  	if (bankersArray[st->id].maxResources == NULL) exit(-1);
	}

  while (looping) {

	  read_communication(socket_r, socket_w, &reading_data);

	  if (reading_data.communication_type == INI) {
		  bool clientExist = false;
			pthread_mutex_lock(&mutex);
		  for (int i = 0; i < nbServerThreads; i++) {
			  if (bankersArray[i].clientID == reading_data.args[0]) {
			    clientExist = true;
			    break;
			  }
		  }
			pthread_mutex_unlock(&mutex);
		  if (clientExist) {
				write_errorMessage(socket_w, "Client already exists");
				looping = false;
			}
			else {
		    for (int i = 0; i < bankersAlgoNbResources; i++) {
	        printf(" %d", reading_data.args[i+1]);
	        bankersArray[st->id].allocatedResources[i] = 0;
	        bankersArray[st->id].maxResources[i] = reading_data.args[i+1];
        }
        bankersArray[st->id].clientID = reading_data.args[0];
		    printf("\n\n");
				pthread_mutex_lock(&mutex);
		    nb_registered_clients++;
				pthread_mutex_unlock(&mutex);
		    writing_data.communication_type = ACK;
		    writing_data.args = NULL;
		    writing_data.args_count = 0;
        write_communication(socket_w, &writing_data);
		    isInitialize = true;
		  }
	  }
	  else if (reading_data.communication_type == REQ) {
			pthread_mutex_lock(&mutex);
			request_processed++;
			pthread_mutex_unlock(&mutex);
		  if (!isInitialize) {
			  printf("\n\nClient not initialized ID : %i\n\n", st->id);
			  write_errorMessage(socket_w, "Client not initialized");
				pthread_mutex_lock(&mutex);
				count_invalid++;
				pthread_mutex_unlock(&mutex);
		  }
		  else {	
		    printf("\n\n---REQ---\n\n");
	      for (int i = 0; i < reading_data.args_count; i++)
	      {
		      printf("%d ",reading_data.args[i]);
	      }
		    printf("\n\n");

		    //REQ
		    bool requestAccepted = true;
				bool valid = true;

		    int * needed = 
				  calloc(nbServerThreads * bankersAlgoNbResources, sizeof(int));
		    if (needed == NULL) exit(-1);
				int* tmpAvail = calloc(bankersAlgoNbResources,sizeof(int));
		    if(tmpAvail == NULL) exit(-1);
        int* tmpAlloc = calloc(bankersAlgoNbResources, sizeof(int));
        if(tmpAlloc == NULL) exit(-1);
				
				pthread_mutex_lock(&mutex);
				memcpy(tmpAvail, bankersAlgoAvailable, 
				        bankersAlgoNbResources * sizeof(int));
		    for (int i = 0; i < bankersAlgoNbResources; i++) {
					int value = bankersArray[st->id].allocatedResources[i] + 
					            reading_data.args[i+1];
          if (value > bankersArray[st->id].maxResources[i] || 
					    value < 0) {
						  //ERROR  : trying to allocate over max or deallocate under 0
              valid = false;
							break;
					}
					tmpAvail[i] -= reading_data.args[i+1];
					tmpAlloc[i] = value;
		    }
				if (valid) {
        	for (int i = 0; i < nbServerThreads; i++) {
						if (bankersArray[i].clientID == -1) {
							//No client assigned, skip
							continue;
						}
						bankersArray[i].visited = false;
						for (int j = 0; j < bankersAlgoNbResources; j++) {
							int index = i * bankersAlgoNbResources + j;
							if (i == st->id) {
								int tmp = bankersArray[st->id].allocatedResources[j];
								bankersArray[st->id].allocatedResources[j] = tmpAlloc[j];
								tmpAlloc[j] = tmp;
							}
              needed[index] = bankersArray[i].maxResources[j] - 
							                bankersArray[i].allocatedResources[j];							
						}
					}
		    	requestAccepted = banker(needed,tmpAvail);	
		    	if (requestAccepted) {	 
			    	//*Counts request as accepted and send ACK message back to client
						for (int i = 0; i < bankersAlgoNbResources; i++) {
							bankersAlgoAvailable[i] -= reading_data.args[i+1];
						}
			    	writing_data.communication_type = ACK;
			    	writing_data.args = NULL;
			    	writing_data.args_count = 0;
			    	write_communication(socket_w, &writing_data);
			    	//*End
			    	count_accepted++;
		    	}
		    	else {
						for (int i = 0; i < bankersAlgoNbResources; i++) {
							//Revert Request
							bankersArray[st->id].allocatedResources[i] = tmpAlloc[i];
						}
						count_wait++;
			    	writing_data.communication_type = WAIT;
			    	writing_data.args_count = 1;
			      writing_data.args = malloc(sizeof(int));
			      if (writing_data.args == NULL) exit(-1);
			      writing_data.args[0] = 1;
			      write_communication(socket_w, &writing_data);
		      }
			  }
				else {
          //Validation
					count_invalid++;
					write_errorMessage(socket_w, "Invalid Request");
				}
				for (int i = 0; i < nbServerThreads; i++) {
					bankersArray[i].visited = false;
				}
				pthread_mutex_unlock(&mutex);
				free(tmpAlloc);
				free(tmpAvail);
			  free(needed);
			}
	  }
	  else if (reading_data.communication_type == CLO) {
		  //CLO
			pthread_mutex_lock(&mutex);
		  if (st_signal(socket_w, st)) {
			  //Exit while
			  clients_ended++;
			  free(bankersArray[st->id].allocatedResources);
			  free(bankersArray[st->id].maxResources);
			  bankersArray[st->id].allocatedResources = NULL;
			  bankersArray[st->id].maxResources = NULL;
			  bankersArray[st->id].visited = false;
			  bankersArray[st->id].clientID = -1;
			  pthread_mutex_unlock(&mutex);
			  looping = false;
		  }
		  else {
			  write_errorMessage(socket_w, 
				  "La commande CLO a été appelée avant la libération de toutes les ressources."
					);
				count_invalid++;
				pthread_mutex_unlock(&mutex);
			}
	  }
	  else if (reading_data.communication_type == END) {
		  printf("\n\nEnd received\n\n");
		  //End server if the received communication is END
		  pthread_mutex_lock(&mutex);
		  if(nb_registered_clients != 0) {
				write_errorMessage(socket_w, "Client(s) are still connected");
				pthread_mutex_unlock(&mutex);
		  } 
		  else {
		    accepting_connections = false;
		    writing_data.communication_type = ACK;
		    writing_data.args_count = 0;
		    writing_data.args = NULL;
		    write_communication(socket_w, &writing_data);
				free(bankersArray[st->id].allocatedResources);
				free(bankersArray[st->id].maxResources);
				free(bankersArray);
				free(bankersAlgoMaxResources);
				free(bankersAlgoAvailable);
				pthread_mutex_unlock(&mutex);
		    looping = false;
		  }  
	  }

	  free(reading_data.args);
	  reading_data.args = NULL;
	  free(writing_data.args);
	  writing_data.args = NULL;
  }

  fclose (socket_r);
  fclose (socket_w);
}

//Called when closing thread
bool
st_signal (FILE * socket_w, server_thread * st)
{
	bool cleanExit = true;
	struct communication_data writing_data;

	for (int i = 0; i < bankersAlgoNbResources; i++) {
		if (bankersArray[st->id].allocatedResources[i] != 0) {
			cleanExit = false;
			break;
		}
	}

	if (cleanExit) {
		nb_registered_clients--;
		count_dispatched++;
		writing_data.communication_type = ACK;
		writing_data.args = NULL;
		writing_data.args_count = 0;
		write_communication(socket_w, &writing_data);
	}

	return cleanExit;
}

int st_wait() {
  struct sockaddr_in thread_addr;
  socklen_t socket_len = sizeof (thread_addr);
  int thread_socket_fd = -1;
  int end_time = time (NULL) + max_wait_time;

  while(thread_socket_fd < 0 && accepting_connections) {
    thread_socket_fd = accept(server_socket_fd,
        (struct sockaddr *)&thread_addr,
        &socket_len);
    if (time(NULL) >= end_time) {
      break;
    }
  }
  return thread_socket_fd;
}

void *
st_code (void *param)
{
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;
  bankersArray[st->id].visited = false;
  bankersArray[st->id].clientID = -1;
  printf("\n\nServer thread %d\n\n", st->id);

  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    thread_socket_fd = st_wait();
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);
      continue;
    }

    if (thread_socket_fd > 0)
    {
      st_process_requests (st, thread_socket_fd);
	  close(thread_socket_fd);
    }
  }
  return NULL;
}


//
// Ouvre un socket pour le serveur.
//
void
st_open_socket (int port_number)
{
  server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket_fd < 0)
    perror ("ERROR opening socket");

  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, 
	    &(int){ 1 }, sizeof(int)) < 0) {
    perror("setsockopt()");
    exit(1);
  }

  struct sockaddr_in serv_addr;
  memset (&serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons (port_number);

  if (bind
      (server_socket_fd, (struct sockaddr *) &serv_addr,
       sizeof (serv_addr)) < 0)
    perror ("ERROR on binding");

  listen (server_socket_fd, server_backlog_size);
}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL) fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du serveur ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
    fprintf (fd, "Requêtes en attente: %d\n", count_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients : %d\n", count_dispatched);
    fprintf (fd, "Requêtes traitées: %d\n", request_processed);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_wait,
        count_invalid, count_dispatched, request_processed);
  }
}


/***** MESSAGE COMMUNICATION PART *****/

//Reads client queries
void read_communication(FILE * socket_r, FILE * socket_w, 
                        struct communication_data * data) {
	bool result = setup_communication_data(socket_r, socket_w, data);
	if (!result)
	{
		printf("\n\n Invalid Request, result : %i, comm_type : %i\n\n", 
		  result, data->communication_type);
		data->args_count = 0;
		data->communication_type = ERR;
		free(data->args);
		data->args = NULL;
		pthread_mutex_lock(&mutex);
		count_invalid++;
		pthread_mutex_unlock(&mutex);
		write_errorMessage(socket_w, "Invalid Request");
	}
}

void write_errorMessage(FILE * socket_w, char * errorMessage) {
	fprintf(socket_w, "ERR %s\n", errorMessage);
	fflush(socket_w);
}

//Writes to client
void write_communication(FILE * socket_w, struct communication_data * data) {
	if (data->communication_type == ACK) {
		fprintf(socket_w, "ACK\n");
	}
	else if (data->communication_type == WAIT) {
		if (data->args_count == 1) {
			fprintf(socket_w, "WAIT %d\n", data->args[0]);
		}
	}
	fflush(socket_w);
}

//Set the data up for lecture
bool setup_communication_data(FILE * socket_r, FILE * socket_w, 
                              struct communication_data * data) {

	char comm_type[3];
	//Reads the communication_type
	fscanf(socket_r, "%3s", comm_type);

	if (strcmp(comm_type, "CLO") == 0) {
		data->communication_type = CLO;
		data->args_count = 0;
		data->args = NULL;
		return (fgetc(socket_r) != '\n');
	}
	if (strcmp(comm_type, "BEG") == 0) {
		 if(fgetc(socket_r) != ' ') return false;
		data->communication_type = BEG;
		data->args = malloc(sizeof(int));
		if (data->args == NULL) exit(-1);
		int count = -1;
		fscanf(socket_r, "%d%n", &data->args[0], &count);
		data->args_count = 1;
		return (count != -1)&&(fgetc(socket_r) == '\n');
	}
	else if (strcmp(comm_type, "PRO") == 0) {
		if(fgetc(socket_r) != ' ') return false;
		data->communication_type = PRO;
		data->args_count = bankersAlgoNbResources;
		data->args = calloc(bankersAlgoNbResources,sizeof(int));
		if (data->args == NULL) exit(-1);
		int i = 0;
		while (i < data->args_count) {
			int count = -1;
			fscanf(socket_r, "%d%n", &data->args[i], &count);
			char c = fgetc(socket_r);
			if(count == -1 || !(c == ' ' || c == '\n')) return false;
			i++;
			if((c == '\n') && (i == data->args_count)) return true;
		}
		return false;
	}
	else if (strcmp(comm_type, "END") == 0) {
		data->communication_type = END;
		data->args_count = 0;
		data->args = NULL;
		return (fgetc(socket_r) == '\n');
	}
	else if (strcmp(comm_type, "INI") == 0) {
		data->communication_type = INI;
		data->args_count = bankersAlgoNbResources + 1;
		data->args = calloc(data->args_count,sizeof(int));
		if (data->args == NULL) exit(-1);
		int i = 0;
		while (i < data->args_count) {
			int count = -1;
			fscanf(socket_r, "%d%n", &data->args[i], &count);
			char c = fgetc(socket_r);
			if(count == -1 || !(c == ' ' || c == '\n')) return false;
			i++;
			if((c == '\n') && (i == data->args_count)) return true;
		}
		return false;
	}
	else if (strcmp(comm_type, "REQ") == 0) {
		data->communication_type = REQ;
		data->args_count = bankersAlgoNbResources + 1;
		data->args = calloc(data->args_count,sizeof(int));
		if (data->args == NULL) exit(-1);
		int i = 0;
		while (i < data->args_count) {
			int count = -1;
			fscanf(socket_r, "%d%n", &data->args[i], &count);
			char c = fgetc(socket_r);
			if(count == -1 || !(c == ' ' || c == '\n')) return false;
			i++;
			if((c == '\n') && (i == data->args_count)) return true;
		}
		return false;
	}
	else if (strcmp(comm_type, "CLO") == 0) {
		data->communication_type = CLO;
		data->args_count = 1;
		data->args = malloc(sizeof(int));
		if (data->args == NULL) exit(-1);
		int count = -1;
		fscanf(socket_r, "%d\n%n", &data->args[0], &count);
		data->args_count = 1;
		return (count != -1);
	}
	else {
		//Invalid Command
		return false;
	}
}

/*
  Banker algorithm

  needed : 1D array of size nbServerThreads (or nbClients) * nbResources
  avaialble : 1D array of size nbResources
	return true if stable, otherwise false
*/
bool banker(int* needed, int* available) {
  bool found;
	do {
    found = false;
		for (int i = 0; i < nbServerThreads; i++) {
			if (bankersArray[i].clientID == -1 ||
			    bankersArray[i].visited)
				continue;//If empty or visted, skip
			
			if (isAvailable(&needed[i*bankersAlgoNbResources], available)) {
				found = true;
				bankersArray[i].visited = true;
				for (int k = 0; k < bankersAlgoNbResources; k++) {
					available[k] += bankersArray[i].allocatedResources[k];
				}
			}
		}
	} while(found);

	//Check if all picked
	bool result = true;
	for (int i = 0; nbServerThreads; i++) {
		if (bankersArray[i].clientID == -1)
		  continue; //if empty, skip
    result = bankersArray[i].visited;
		if (!result) break;
	}
  return result;
}

/*Utility function
	compare needed and available 1D array
*/
bool isAvailable(int* needed, int* available) {
  for(int i = 0; i < bankersAlgoNbResources; i++) {
	if(needed[i] > available[i])
	  return false;
  }
  return true;
}