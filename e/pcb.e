#ifndef PCB
#define PCB

/************************* PROCQ.E *****************************
*
*  The externals declaration file for the Process Control Block
*    Module.
*
*  Written by Mikeyg
*/

#include "../h/types.h"

extern void freePcb (pcb_PTR p);
extern pcb_PTR allocPcb ();
extern void initPcbs ();

/* creates and returns an empty process block */
extern pcb_PTR mkEmptyProcQ ();
/* checks whether given process queue is empty */
extern int emptyProcQ (pcb_PTR tp);
/* inserts a given process block into the process queue 
 * whose tail pointer is provided */
extern void insertProcQ (pcb_PTR *tp, pcb_PTR p);
/* dequeues head process block from queue referenced by 
 * tp */
extern pcb_PTR removeProcQ (pcb_PTR *tp);
/* searches for provided process block in the queue
 * referenced by tp. if found, removes it from the queue */
extern pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p);
/* returns the head of the queue referenced by tp without
 * removing it from the queue */
extern pcb_PTR headProcQ (pcb_PTR tp);

extern int emptyChild (pcb_PTR p);
extern void insertChild (pcb_PTR prnt, pcb_PTR p);
extern pcb_PTR removeChild (pcb_PTR p);
extern pcb_PTR outChild (pcb_PTR p);

/***************************************************************/

#endif
