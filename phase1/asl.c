/*********************************asl.c************************************/
/*          Active Semaphore List for the Kaya Operating System.          */
/*   The methods in this file will manage the free and active semaphores  */
/*              By: Chris Staudigel and Grant Stapleton                   */
/*              with starting code by Michael Goldweber                   */
/**************************************************************************/

#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"


semd_PTR semdFree_h,        /* Head of free list stack */
         semdActive_h;      /* Head of ASL list ordered by memory address */

/* ========================== Helper Methods ========================== */

/*
 * pull semd from freelist and add it to active list
 */
HIDDEN semd_PTR allocSemd()
{
    semd_PTR returnMe;

    /* no free semds, error */
    if (semdFree_h == NULL)
    {
        return NULL;
    }

    /* using first element in free list */
    returnMe = semdFree_h;
    semdFree_h = semdFree_h -> s_next;

    returnMe -> s_next = NULL;
    returnMe -> s_semAdd = NULL;
    returnMe -> s_procQ = mkEmptyProcQ();


    return returnMe;
}

/*
 * add semd to free list stack
 */
HIDDEN void freeSemd(semd_PTR removing) 
{
    /* push onto the stack */
    removing -> s_next = semdFree_h;
    semdFree_h = removing;
}

/*
 * searches the ASL for the provided node and returns the 
 * node PREVIOUS to the found (or not) node
 * ex: 10->20->30
 * searching for 30 returns 20
 * searching for 25 returns 20
 */
HIDDEN semd_PTR searchASL(int *semAdd)
{
    /* starting with the head of the list */
    semd_PTR searching;
    searching = semdActive_h;

    /* error case, return the last real element in the stack */
    if (semAdd == NULL)
    {
        semAdd = (int*) MAXINT;
    }


    /* while the next node's address < searching for address */
    while (searching -> s_next -> s_semAdd < semAdd)
    {
        /* check next node */
        searching = searching -> s_next;
    }

    return searching; 
}

/*
 * searches semdlist for semAdd
 * 2 cases
 *   Found -> insertProcQ(semAdd->tp, p)
 *   Not Found -> 
 *        new semd_t with empty tp
 *        add new semd_t into ASL (Sorted by s_semAdd)
 *        same process as found
 */
int insertBlocked (int *semAdd, pcb_PTR p)
{
    /* find the parent of the new node */
    semd_PTR q;
    q = searchASL(semAdd);
    
    /* semaphore already exists */
    if (q -> s_next -> s_semAdd == semAdd)
    {
        p -> pcb_semAdd = semAdd;
        insertProcQ(&(q -> s_next -> s_procQ), p);
        return FALSE;
    }


    /* allocate new semaphore */
    semd_PTR new;
    new = allocSemd();
    /* there arent any free semaphores to use, error */
    if (new == NULL)
    {
        return TRUE;
    }


    /* weave new semaphore onto active list */
    new -> s_next = q -> s_next;
    q -> s_next = new;

    /* insert procQ onto semaphore */
    new -> s_procQ = mkEmptyProcQ();
    insertProcQ(&(new -> s_procQ), p);

    /* set semAdd */
    new -> s_semAdd = semAdd;
    p -> pcb_semAdd = semAdd;

    return FALSE;


}

/*
 * search ASL for given semAdd
 * 2 cases:
 *   not found: error case (return null)?
 *   found: removeProcQ on s_procQ returns pcb_PTR
 *     2 cases:
 *          processQueue is not empty -> done
 *          processQueue is Empty ->
 *              deallocate semd node
 */
pcb_PTR removeBlocked (int *semAdd)
{
    /* find parent of semaphore we are removing */
    semd_PTR prev;
    prev = searchASL(semAdd);

    /* semadd is not in the list, error case */
    if (prev -> s_next -> s_semAdd != semAdd)
    {
        return NULL;
    }


    pcb_PTR p;
    /* remove dequeue first process from semaphore */
    p = removeProcQ(&prev -> s_next -> s_procQ);

    /* if procQ is now empty */
    if(emptyProcQ(prev -> s_next -> s_procQ))
    {
        /* remove semaphore from active list */
        semd_PTR removing;
        removing = prev -> s_next;

        prev -> s_next = removing -> s_next;
        freeSemd(removing);
        removing -> s_semAdd = NULL;
    }

    /* reset the semAdd if the removed process block exists */
    if (p != NULL)
    {
        p -> pcb_semAdd = NULL;
    }
    
    return p;
}

/*
 * Remove the ProcBlk pointed to bypfrom the process queue associated 
 * with p’s semaphore (p→psemAdd) on the ASL. If ProcBlk pointed to by p 
 * does not appear in the process queue associated with p’s semaphore, 
 * which is an error condition, return NULL; otherwise,return p. 
 */
pcb_PTR outBlocked (pcb_PTR p)
{
    semd_PTR prev;
    /* find parent */
    prev = searchASL(p -> pcb_semAdd);

    /* error case, the semaphore is not on the list */
    if (prev -> s_next -> s_semAdd != p -> pcb_semAdd)
    {
        return NULL;
    }


    /* pull pcb from procQ */
    pcb_PTR pcb;
    pcb = outProcQ(&(prev -> s_next -> s_procQ), p);

    /* if procQ is empty */
    if (emptyProcQ(prev -> s_next -> s_procQ))
    {
        /* remove semaphore and place it on the free list */
        semd_PTR removing;
        removing = prev -> s_next;

        prev -> s_next = prev -> s_next -> s_next;
    
        freeSemd(removing);
        removing -> s_semAdd = NULL;
    }

    /* reset the semadd on process block */
    pcb -> pcb_semAdd = NULL;
    return pcb;
    
}

/*
 * Return a pointer to the ProcBlk that is at the head of the process 
 * queue associated with the semaphore semAdd. Return NULL if semAdd is 
 * not found on the ASL or if the process queue associated with semAdd is empty. 
 */
pcb_PTR headBlocked (int *semAdd)
{
    /* find parent */
    semd_PTR prev;
    prev = searchASL(semAdd);

    return headProcQ(prev -> s_next -> s_procQ);
}

/* 
 * Initialize the semdFree list to contain all the elements of the array 
 * static semdt semdTable[MAXPROC] This method will be only called once 
 * during data structure initialization. 
 */
void initASL()
{
    /* variables were using */
    int i;
    semdFree_h = NULL;
    semdActive_h = NULL;
    HIDDEN semd_t semdTable[MAXPROC+2];
    for(i=0;i<MAXPROC;++i)
    {
        /* free each semaphore */
        freeSemd(&(semdTable[i]));
    }

    /* dummy nodes */
    semd_PTR semdZero;
    semd_PTR semdMax;

    /* initialization */
    semdZero = &(semdTable[MAXPROC]);
    semdMax = &(semdTable[MAXPROC+1]);
  
    /* set values */
    semdZero -> s_semAdd = 0;
    semdMax -> s_semAdd = MAXINT;
    semdZero -> s_procQ = mkEmptyProcQ();
    semdMax -> s_procQ = mkEmptyProcQ();

    /* add to active list */
    semdZero -> s_next = semdMax;
    semdActive_h = semdZero;
}
