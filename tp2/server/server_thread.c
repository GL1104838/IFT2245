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

//Added int
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

	// Initialise le nombre de clients connecté.
	nb_registered_clients = 0;

	nbServerThreads = threadsCount;
	bankersArray = malloc(threadsCount * sizeof(struct bankersArray));

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

	//BEG
	read_communication(socket_r, socket_w, &reading_data);
	while (reading_data.communication_type != BEG) {
		read_communication(socket_r, socket_w, &reading_data);
	}
	if (reading_data.communication_type == BEG) {
		printf("\n\n---BEG---\n\n");
	}
	writing_data.communication_type = ACK;
	write_communication(socket_w, &writing_data);
	bankersAlgoNbResources = reading_data.args[0];
	//PRO
	read_communication(socket_r, socket_w, &reading_data);
	while (reading_data.communication_type != PRO) {
		read_communication(socket_r, socket_w, &reading_data);
	}
	if (reading_data.communication_type == PRO) {
		printf("\n\n---PRO---\n\n");
	}
	printf("\n\nPRO");
	bankersAlgoAvailable = calloc(bankersAlgoNbResources, sizeof(int));
	bankersAlgoMaxResources = calloc(bankersAlgoNbResources, sizeof(int));
	for (int i = 0; i < reading_data.args_count; i++) {
		printf(" %d", reading_data.args[i]);
		bankersAlgoMaxResources[i] = reading_data.args[i];
		bankersAlgoAvailable[i] = reading_data.args[i];
	}
	printf("\n\n");
	writing_data.communication_type = ACK;
	write_communication(socket_w, &writing_data);

	fclose(socket_r);
	fclose(socket_w);
	close(socket);
}

void
st_process_requests (server_thread * st, int socket_fd)
{
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  bankersArray[st->id].allocatedResources = calloc(bankersAlgoNbResources, sizeof(int));
  bankersArray[st->id].maxResources = calloc(bankersAlgoNbResources, sizeof(int));

  bool isInitialize = false;
  struct communication_data reading_data;
  struct communication_data writing_data;
  
  while (true) {

	  read_communication(socket_r, socket_w, &reading_data);

	  if (reading_data.communication_type == INI) {
		  for (int i = 0; i < bankersAlgoNbResources; i++) {
	        printf(" %d", reading_data.args[i+1]);
	        bankersArray[st->id].allocatedResources[i] = reading_data.args[i+1];
	        bankersArray[st->id].maxResources[i] = bankersAlgoMaxResources[i];
          }
		  printf("\n\n");
		  pthread_mutex_lock(&mutex);
		  nb_registered_clients++;
		  pthread_mutex_unlock(&mutex);
		  writing_data.communication_type = ACK;
		  writing_data.args_count = 0;
          write_communication(socket_w, &writing_data);
		  isInitialize = true;
	  }
	  else if (reading_data.communication_type == REQ) {
		  if (!isInitialize) {
			  printf("\n\nClient not initialized ID : %i\n\n", st->id);
			  write_errorMessage(socket_w, "Client not initialized");
			  //free(reading_data.args);
			  //reading_data.args = NULL;
			  continue;
		  }	
		  printf("\n\n---REQ---\n\n");
	      for (int i = 0; i < reading_data.args_count; i++)
	      {
		    printf("%d,",reading_data.args[i]);
	      }
		  printf("\n\n");
		  //****************************************************************************************
		  //REQ
		  //Ce qu'il reste à faire:
		  //Vérifier si la requête est valide
		  //Ajuster les envoi err/ack/wait
		  //Effectuer la gestion des ressources du banquier
		  //J'utilise les variables globales pour la structure générale du banquier afin qu'ils soient partagées entre tous les threads
		  //et un bankersArray[i] où i = st->id selon le thread
		  bool requestAccepted = true;
		  bool requestProcessed = false;

		  int * neededResources = calloc(bankersAlgoNbResources, sizeof(int));
		  //NegativeData sont les resources négatives(à libérer) et Positive celles à allouer
		  int * negativeData = calloc(bankersAlgoNbResources, sizeof(int));
		  int * positiveData = calloc(bankersAlgoNbResources, sizeof(int));
		  
		  for (int i = 0; i < bankersAlgoNbResources; i++) {
			  if(reading_data.args[i+1] > 0){
				  positiveData[i] = reading_data.args[i + 1];
			  }
			  else {
				  negativeData[i] = reading_data.args[i + 1];
			  }
			  neededResources[i] = bankersArray[st->id].maxResources[i] - bankersArray[st->id].allocatedResources[i];
		  }

		  if (requestAccepted) {

			  pthread_mutex_lock(&mutex);

			  for (int i = 0; i < bankersAlgoNbResources; i++) {
				  //Reallocate negative data
				  bankersArray[st->id].allocatedResources[i] -= negativeData[i];
				  bankersAlgoAvailable[i] += negativeData[i];
			  }
			  
			 
			  //*Counts request as accepted and send ACK message back to client
			  count_accepted++;
			  writing_data.communication_type = ACK;
			  write_communication(socket_w, &writing_data);
			  //*End

			  requestProcessed = true;
			  
			  pthread_mutex_unlock(&mutex);
		  }
		  else {
			  //request is waiting [Il faut probablement changer ça]
			  writing_data.communication_type = WAIT;
			  writing_data.args_count = 1;
			  writing_data.args = malloc(sizeof(int));
			  writing_data.args[0] = 1;
			  write_communication(socket_w, &writing_data);

			  pthread_mutex_lock(&mutex);
			  count_invalid++;
			  pthread_mutex_unlock(&mutex);
		  }
		  if (requestProcessed) {
			  //request was processed [Il faut probablement changer ça]
			  pthread_mutex_lock(&mutex);
			  request_processed++;
			  pthread_mutex_unlock(&mutex);
		  }
		  //**************************************************************************************************
	  }
	  else if (reading_data.communication_type == CLO) {
		  //CLO
		  if (st_signal(socket_w, st)) {
			  //Exit while
			  pthread_mutex_lock(&mutex);
			  clients_ended++;
			  pthread_mutex_unlock(&mutex);
			  break;
		  }
		  else {
			  write_errorMessage(socket_w, "La commande CLO a été appelée avant la libération de toutes les ressources.");
		  }
	  }
	  else if (reading_data.communication_type == END) {
		printf("\n\nEnd received\n\n");
		//End server if the received communication is END
		pthread_mutex_lock(&mutex);
		//TODO : Check if resources are all back
		accepting_connections = false;
		pthread_mutex_unlock(&mutex);
		writing_data.communication_type = ACK;
		writing_data.args_count = 0;
		write_communication(socket_w, &writing_data);
		break;
	  }
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

	//Verifies if closing properly
	/*for (int i = 0; i < bankersAlgoNbResources; i++) {
		if (!(bankersArray[st->id].allocatedResources[i] == 0)) {
			cleanExit = false;
		}
	}*/

	if (cleanExit) {
		pthread_mutex_lock(&mutex);
		nb_registered_clients--;
		count_dispatched++;
		pthread_mutex_unlock(&mutex);
		writing_data.communication_type = ACK;
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

  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
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
void read_communication(FILE * socket_r, FILE * socket_w, struct communication_data * data) {
	bool result = setup_communication_data(socket_r, socket_w, data);
	if (!result)
	{
		printf("\n\n Setup ERROR, result : %i, comm_type : %i\n\n", result, data->communication_type);
		data->args_count = 0;
		data->communication_type = ERR;
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
bool setup_communication_data(FILE * socket_r, FILE * socket_w, struct communication_data * data) {

	char comm_type[3];
	//Reads the communication_type
	fscanf(socket_r, "%3s", comm_type);

	if (strcmp(comm_type, "CLO") == 0) {
		data->communication_type = CLO;
		data->args_count = 0;
		return (fgetc(socket_r) != '\n');
	}
	if (strcmp(comm_type, "BEG") == 0) {
		 if(fgetc(socket_r) != ' ') return false;
		data->communication_type = BEG;
		data->args = malloc(sizeof(int));
		int count = -1;
		fscanf(socket_r, "%d%n", &data->args[0], &count);
		data->args_count = 1;
		return (count != -1) && (fgetc(socket_r) == '\n');
	}
	else if (strcmp(comm_type, "PRO") == 0) {
		if(fgetc(socket_r) != ' ') return false;
		data->communication_type = PRO;
		data->args_count = bankersAlgoNbResources;
		data->args = malloc(data->args_count * sizeof(int));
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
		return (fgetc(socket_r) == '\n');
	}
	else if (strcmp(comm_type, "INI") == 0) {
		data->communication_type = INI;
		data->args_count = bankersAlgoNbResources + 1;
		data->args = malloc(data->args_count * sizeof(int));
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
		data->args = malloc(data->args_count * sizeof(int));
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