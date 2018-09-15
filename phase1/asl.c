#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"


semd_PTR semdFree_h, /* Head of free list */
         semdActive_h;     /* Head of ASL */

/* ========================== Helper Methods ========================== */
HIDDEN semd_PTR searchASL(int *semAdd);

/*
 * pull semd from freelist and add it to active list
 */
HIDDEN semd_PTR allocSemd()
{
    semd_PTR returnMe;

    /* no free semds */
    if (semdFree_h == NULL)
    {
        returnMe = NULL;
    }
    else
    {
        /* using first element in free list */
        returnMe = semdFree_h;
        semdFree_h = semdFree_h -> s_next;

        returnMe -> s_next = NULL;
        returnMe -> s_semAdd = NULL;
        returnMe -> s_procQ = mkEmptyProcQ();
        
    }

    return returnMe;
}

/*
 * add semd to free list
 */
HIDDEN void freeSemd(semd_PTR removing) 
{
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
    semd_PTR searching;
    searching = semdActive_h;

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
    semd_PTR q;
    q = searchASL(semAdd);
    
    /* semaphore already exists */
    if (q -> s_next -> s_semAdd == semAdd)
    {
        p -> pcb_semAdd = semAdd;
        insertProcQ(&(q -> s_next -> s_procQ), p);
        return FALSE;
    }
    else
    {
        /* allocate new semaphore */
        semd_PTR new;
        new = allocSemd();
        /* there arent any free semaphores to use */
        if (new == NULL)
        {
            return TRUE;
        }
        else
        {
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
    }
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
    semd_PTR parent;
    parent = searchASL(semAdd);

    pcb_PTR removing;
    removing = headProcQ(parent -> s_next -> s_procQ);

    return outBlocked(removing);
}

/*
 * same as removeBlocked except call outProcQ instead of removeProcQ
 */
pcb_PTR outBlocked (pcb_PTR p)
{
    semd_PTR prev;

    prev = searchASL(p -> pcb_semAdd);

    if (prev -> s_next -> s_semAdd == p -> pcb_semAdd)
    {
        pcb_PTR pcb;
        pcb = outProcQ(&(prev -> s_next -> s_procQ), p);

        if (emptyProcQ(prev -> s_next -> s_procQ))
        {
            semd_PTR removing;
            removing = prev -> s_next;

            prev -> s_next = prev -> s_next -> s_next;
        
            freeSemd(removing);
            removing -> s_semAdd = NULL;
        }
        pcb -> pcb_semAdd = NULL;
        return pcb;
    }
    else
    {  
        return NULL;
    }
    
}

/*
 * same as remove/out except call headProcQ and do not deallocate
 */
pcb_PTR headBlocked (int *semAdd)
{
    semd_PTR prev;
    prev = searchASL(semAdd);
    if (prev -> s_next -> s_semAdd == semAdd)
    {
        return headProcQ(prev -> s_next -> s_procQ);
    }
    return NULL;
}

/*
 * create ASL of length MAXPROC + 2 dummy nodes
 */
void initASL()
{
    int i;
    semdFree_h = NULL;
    semdActive_h = NULL;
    HIDDEN semd_t semdTable[MAXPROC+2];
    for(i=0;i<MAXPROC;++i)
    {
        freeSemd(&(semdTable[i]));
    }

    semd_PTR semdZero;
    semd_PTR semdMax;

    semdZero = &(semdTable[MAXPROC]);
    semdMax = &(semdTable[MAXPROC+1]);
  
    semdZero -> s_semAdd = 0;
    semdMax -> s_semAdd = MAXINT;
    semdZero -> s_procQ = mkEmptyProcQ();
    semdMax -> s_procQ = mkEmptyProcQ();

    semdZero -> s_next = semdMax;

    semdActive_h = semdZero;
}
