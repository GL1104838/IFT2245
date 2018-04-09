
#include <stdint.h>
#include <stdio.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;             /* Invalide si négatif.  */
  bool readonly : 1;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

//Added global FIFO_queue
static int FIFO_queue[NUM_FRAMES];

/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
    tlb_entries[i].frame_number = -1;
  tlb_log = log;

  //ADDED FIFO_queue initialization
  for (int i = 0; i < NUM_FRAMES; i++) {
	  FIFO_queue[i] = -1;
  }
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
		if (tlb_entries[i].page_number == page_number) {
			return tlb_entries[i].frame_number;
		}
	}

  return -1;
}

//Return if the entry is read only
bool tlb_getEntryIsReadOnly(int page_number) {
	for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
		if (tlb_entries[i].page_number == page_number) {
			if (tlb_entries[i].readonly) {
				return true;
			}
		}
	}
	return false;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
  // TODO: COMPLÉTER CETTE FONCTION.
	bool forceEnd = false;
	for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
		bool isFrameNumber = (tlb_entries[i].frame_number == frame_number);
		if (isFrameNumber || tlb_entries[i].frame_number == -1) {
			//Rewrite or add
			if (isFrameNumber) {
				//Rewriting, remove the entry from the the FIFO_queue
				tlb__removeFIFO(i);
			}

			tlb_entries[i].frame_number = frame_number;
			tlb_entries[i].page_number = page_number;
			tlb_entries[i].readonly = readonly;
			tlb__pushFIFO(i);
			i = TLB_NUM_ENTRIES;
			forceEnd = true;
		}
	}
	if (!forceEnd) {
		//Use the FIFO strategy
		int first = tlb__popFIFO();
		tlb_entries[first].frame_number = frame_number;
		tlb_entries[first].page_number = page_number;
		tlb_entries[first].readonly = readonly;
		tlb__pushFIFO(first);
	}
}

/************** FIFO **************/
//Returns the first element of FIFO and move them all back
int tlb__popFIFO() {
	int returnValue = FIFO_queue[0];
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (i + 1 == NUM_FRAMES) {
			FIFO_queue[i] = -1;
		}
		else {
			FIFO_queue[i] = FIFO_queue[i + 1];
		}
	}
	return returnValue;
}

//Add value to the end of the queue
void tlb__pushFIFO(int value) {
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (FIFO_queue[i] == -1) {
			FIFO_queue[i] = value;
			i = NUM_FRAMES;
		}
	}
}

//Remove a value from the queue
//This is only to be used when overwriting a frame by it's number
void tlb__removeFIFO(int value) {
	for (int i = 0; i < NUM_FRAMES; i++) {
		if (FIFO_queue[i] == value) {
			for (int j = i; j < NUM_FRAMES; j++) {
				if (j + 1 == NUM_FRAMES) {
					FIFO_queue[j] = -1;
				}
				else {
					FIFO_queue[j] = FIFO_queue[j + 1];
				}
			}
			i = NUM_FRAMES;
		}
	}
}
/**********************************/

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
                    unsigned int frame_number, bool readonly)
{
  tlb_mod_count++;
  tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb__lookup (page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}
