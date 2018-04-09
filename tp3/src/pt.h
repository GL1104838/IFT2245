#ifndef PT_H
#define PT_H

#include <stdio.h>
#include <stdbool.h>

/* Initialise la page table, et indique où envoyer le log des accès.  */
void pt_init (FILE *log);

/* Recherche dans la page table.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
int pt_lookup (unsigned int page_number);

/* Modifie l'entrée de `page_number` dans la page table pour qu'elle
 * pointe vers `frame_number`.  */
void pt_set_entry (unsigned int page_number, unsigned int frame_number);

/* Marque l'entrée de `page_number` dans la page table comme invalide.  */
void pt_unset_entry (unsigned int page_number);

/* Renvoie si `page_number` est `readonly`.  */
bool pt_readonly_p (unsigned int page_number);

/* Retourne la première frame libre ou -1 s'il n'y en a pas. */
int pt_get_firstfree();

/* Retournes la première frame de FIFO. */
int pt_peekFrame();

/* Retournes la première page de FIFO. */
int pt_peekPage();

/* Supprimes le premier élément de FIFO */
void pt_popFIFO();

/* Retournes la valeur booléenne isDirty du page_number. */
bool pt_getDirty(int);

/* Assigne la valeur booléene isDirty à page_number */
void pt_setDirty(int, bool);

/* Change l'accès en écriture de `page_number` selon `readonly`.  */
void pt_set_readonly (unsigned int page_number, bool readonly);

void pt_clean (void);

#endif
