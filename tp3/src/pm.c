#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
  pm_backing_store = backing_store;
  pm_log = log;
  memset (pm_memory, '\0', sizeof (pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
  download_count++;
  /* ¡ TODO: COMPLÉTER ! */
  FILE * buffer = pm_backing_store;
  int counter = 0;
  for (int i = 0; i < PAGE_FRAME_SIZE * (page_number+1); i++) {
	  if (i >= PAGE_FRAME_SIZE * page_number) {
		  pm_memory[PAGE_FRAME_SIZE * frame_number + counter] = fgetc(buffer);
		  counter++;
	  }
	  else {
		  fgetc(buffer);
	  }
  }
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page (unsigned int frame_number, unsigned int page_number)
{
  backup_count++;
  /* ¡ TODO: COMPLÉTER ! */

  //INSPIRED BY
  //https://stackoverflow.com/questions/19681898/
  //how-to-write-in-file-at-specific-location
  fseek(pm_backing_store, PAGE_FRAME_SIZE*page_number, SEEK_SET);
  char array[PAGE_FRAME_SIZE];
  for (int i = 0; i < PAGE_FRAME_SIZE; i++) {
	  array[i] = pm_memory[PAGE_FRAME_SIZE * frame_number + i];
  }
  fputs(array, pm_backing_store);

  fflush(pm_backing_store);
}

char pm_read (unsigned int physical_address)
{
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */
  return pm_memory[physical_address];
  //return '!';
}

void pm_write (unsigned int physical_address, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */
  pm_memory[physical_address] = c;
}


void pm_clean (void)
{
  // Enregistre l'état de la mémoire physique.
  if (pm_log)
    {
      for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
	{
	  if (i % 80 == 0)
	    fprintf (pm_log, "%c\n", pm_memory[i]);
	  else
	    fprintf (pm_log, "%c", pm_memory[i]);
	}
    }
  fprintf (stdout, "Page downloads: %2u\n", download_count);
  fprintf (stdout, "Page backups  : %2u\n", backup_count);
  fprintf (stdout, "PM reads : %4u\n", read_count);
  fprintf (stdout, "PM writes: %4u\n", write_count);
}
