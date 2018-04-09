
#include <stdint.h>
#include <stdio.h>

#include "pt.h"

#include "conf.h"

struct page
{
  bool readonly : 1;
  bool valid : 1;
  unsigned int frame_number : 16;

  //Added variable
  bool isDirty;
};

static FILE *pt_log = NULL;
static struct page page_table[NUM_PAGES];

static unsigned int pt_lookup_count = 0;
static unsigned int pt_page_fault_count = 0;
static unsigned int pt_set_count = 0;

//ADDED FIFO_queue, [0] are frames, [1] are their corresponding pages
static int FIFO_queue[NUM_FRAMES][2];

//ADDED freeFrameList, true means it is free, false means it isn't
static bool freeFrameList[NUM_FRAMES];

/* Initialise la table des pages, et indique où envoyer le log des accès.  */
void pt_init (FILE *log)
{
  for (unsigned int i; i < NUM_PAGES; i++)
    page_table[i].valid = false;
  pt_log = log;

  //ADDED array initialization
  for (int i = 0; i < NUM_FRAMES; i++) {
	  FIFO_queue[i][0] = -1;
	  FIFO_queue[i][1] = -1;
	  freeFrameList[i] = true;
  }
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans la table des pages.
 * Renvoie le `frame_number`, si valide, ou un nombre négatif sinon.  */
static int pt__lookup (unsigned int page_number)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	if (page_table[page_number].valid) {
		return page_table[page_number].frame_number;
	}
  return -1;
}

/* Modifie l'entrée de `page_number` dans la page table pour qu'elle
 * pointe vers `frame_number`.  */
static void pt__set_entry (unsigned int page_number, unsigned int frame_number)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	freeFrameList[frame_number] = false;
	page_table[page_number].frame_number = frame_number;
	page_table[page_number].valid = true;
	pt_set_readonly(page_number, false);
	pt_setDirty(page_number, false);
	pt__pushFIFO(frame_number, page_number);
}

/* Marque l'entrée de `page_number` dans la page table comme invalide.  */
void pt_unset_entry (unsigned int page_number)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	freeFrameList[page_table[page_number].frame_number] = true;
	page_table[page_number].valid = false;
}

/* Renvoie si `page_number` est `readonly`.  */
bool pt_readonly_p (unsigned int page_number)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	return page_table[page_number].readonly;
}

/* Change l'accès en écriture de `page_number` selon `readonly`.  */
void pt_set_readonly (unsigned int page_number, bool readonly)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	page_table[page_number].readonly = readonly;
}

//Set page_number's isDirty value to isDirty argument
void pt_setDirty(int page_number, bool isDirty) {
	page_table[page_number].isDirty = isDirty;
}

//Get page_number's isDirty value
bool pt_getDirty(int page_number) {
	return page_table[page_number].isDirty;
}

//ADDED: Returns the first free frame encountered or -1 if none are free
int pt_get_firstfree() {
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (freeFrameList[i]) {
			return i;
		}
	}
	return -1;
}

/************** FIFO **************/
//Deletes the first element of FIFO and move them all back
void pt_popFIFO() {
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (i + 1 == NUM_FRAMES) {
			FIFO_queue[i][0] = -1;
			FIFO_queue[i][1] = -1;
		}
		else {
			FIFO_queue[i][0] = FIFO_queue[i + 1][0];
			FIFO_queue[i][1] = FIFO_queue[i + 1][1];
		}
	}
}

//Add parameters to the end of the queue
void pt__pushFIFO(int frameIndex, int pageIndex) {
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (FIFO_queue[i][0] == -1) {
			FIFO_queue[i][0] = frameIndex;
			FIFO_queue[i][1] = pageIndex;
			i = NUM_FRAMES;
		}
	}
}

//returns the first frame of FIFO
int pt_peekFrame() {
	return FIFO_queue[0][0];
}

//returns the first page of FIFO
int pt_peekPage() {
	return FIFO_queue[0][1];
}
/**********************************/

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void pt_set_entry (unsigned int page_number, unsigned int frame_number)
{
  pt_set_count++;
  pt__set_entry (page_number, frame_number);
}

int pt_lookup (unsigned int page_number)
{
  pt_lookup_count++;
  int fn = pt__lookup (page_number);
  if (fn < 0) pt_page_fault_count++;
  return fn;
}

/* Imprime un sommaires des accès.  */
void pt_clean (void)
{
  fprintf (stdout, "PT lookups   : %3u\n", pt_lookup_count);
  fprintf (stdout, "PT changes   : %3u\n", pt_set_count);
  fprintf (stdout, "Page Faults  : %3u\n", pt_page_fault_count);

  if (pt_log)
    {
      for (unsigned int i = 0; i < NUM_PAGES; i++)
	{
	  fprintf (pt_log,
		   "%d -> {%d,%s%s}\n",
		   i,
		   page_table[i].frame_number,
                   page_table[i].valid ? "" : " invalid",
                   page_table[i].readonly ? " readonly" : "");
	}
    }
}
