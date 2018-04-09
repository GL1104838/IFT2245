#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
   fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */
  int pageIndex = getPageIndex(laddress);
  int frameIndex = getFrameIndex(laddress, pageIndex, false);
  int pageOffset = getPageOffset(laddress);
  int physicalAddress = getPhysicalAddress(laddress, frameIndex, pageOffset);

  c = pm_read(physicalAddress);

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING", laddress, pageIndex, 
	  frameIndex, pageOffset, physicalAddress, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */
  int pageIndex = getPageIndex(laddress);
  int frameIndex = getFrameIndex(laddress, pageIndex, true);
  int pageOffset = getPageOffset(laddress);
  int physicalAddress = getPhysicalAddress(laddress, frameIndex, pageOffset);

  pm_write(physicalAddress, c);

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "WRITING", laddress, pageIndex, 
	  frameIndex, pageOffset, physicalAddress, c);
}

/************** GET **************/
//page 411
int getFrameIndex(int laddress, int pageIndex, bool writing) {
	int frameIndex = tlb_lookup(pageIndex, writing);
	if (frameIndex != -1) {
		//TLB HIT
		if (writing) {
			if (tlb_getEntryIsReadOnly(pageIndex)) {
				//READ ONLY
				frameIndex = pt_lookup(pageIndex);
				tlb_add_entry(pageIndex, frameIndex, false);
				pt_setDirty(pageIndex, true);
				pt_set_readonly(pageIndex, false);
			}
		}
		return frameIndex;
	}
	else {
		//TLB MISS
		frameIndex = pt_lookup(pageIndex);
		if (frameIndex != -1) {
			//FOUND FRAME IN PAGETABLE
			if (pt_readonly_p(pageIndex) && writing) {
				//IS WRITING AND READONLY
				tlb_add_entry(pageIndex, frameIndex, false);
				pt_setDirty(pageIndex, true);
				pt_set_readonly(pageIndex, false);
			}
			else {
				//NOT(WRITING AND READONLY)
				tlb_add_entry(pageIndex, frameIndex, true);
			}
		}
		else {
			//NOT IN PAGETABLE
			frameIndex = pt_get_firstfree();
			if (frameIndex == -1) {
				//NO FREE FRAME LEFT
				frameIndex = pt_peekFrame();
				int selectedPage = pt_peekPage();
				pt_popFIFO();
				if (pt_getDirty(selectedPage)) {
					//SELECTED PAGE IS DIRTY
					pm_backup_page(frameIndex, selectedPage);
				}
				pm_download_page(pageIndex, frameIndex);
				pt_unset_entry(selectedPage);
				tlb_add_entry(pageIndex, frameIndex, true);
				pt_set_entry(pageIndex, frameIndex);
				pt_set_readonly(pageIndex, true);
			}
			else {
				//FOUND FREE FRAME
				tlb_add_entry(pageIndex, frameIndex, true);
				pt_set_entry(pageIndex, frameIndex);
				pt_set_readonly(pageIndex, true);
			}
		}

		return frameIndex;
	}
}

int getPageIndex(int laddress) {
	return laddress / PAGE_FRAME_SIZE;
}

int getPageOffset(int laddress) {
	return laddress % PAGE_FRAME_SIZE;
}

int getPhysicalAddress(int laddress, int frameIndex, int pageOffset) {
	return frameIndex * PAGE_FRAME_SIZE + pageOffset;
}
/*********************************/

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
