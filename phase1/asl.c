#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"


semd_PTR semdFree_h, /* Head of free list */
         semdActive_h;     /* Head of ASL */

void debugA(int *a, int *b)
{
    int i;
    i = 1;
}
void debugB(semd_PTR a, semd_PTR b)
{
    int i;
    i = 1;
}
void debugC(pcb_PTR a, pcb_PTR b)
{
    int i;
    i =0;
}

HIDDEN semd_PTR searchASL(int *semAdd);

HIDDEN semd_PTR allocSemd(int *semAdd)
{
    semd_PTR returnMe;

    if (semdFree_h == NULL)
    {
        returnMe = NULL;
    }
    else
    {
        returnMe = semdFree_h;
        semdFree_h = semdFree_h -> s_next;

        returnMe -> s_next = NULL;
        returnMe -> s_procQ = NULL;
        returnMe -> s_semAdd = semAdd;
    }

    return returnMe;
}

HIDDEN void freeSemd(semd_PTR s) 
{
    if (s -> s_next != NULL) /* jesus maneuver */
    {
        addokbuf("freeing....");
        s -> s_next = s ->  s_next -> s_next;
        addokbuf("freed\n");
    }
    s -> s_next = semdFree_h;
    semdFree_h = s;
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
    while (searching -> s_next -> s_semAdd < semAdd)
    {
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

    if (q -> s_next -> s_semAdd == semAdd)
    {
        p -> pcb_semAdd = semAdd;
        insertProcQ(&q -> s_procQ, p);
        return FALSE;
    }
    else
    {
        semd_PTR new;
        new = allocSemd(semAdd);
        if (new == NULL)
        {
            return TRUE;
        }
        else
        {
            new -> s_next = q -> s_next;
            q -> s_next = new;
            new -> s_procQ = mkEmptyProcQ();
            insertProcQ(&new -> s_procQ, p);
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
    semd_PTR prev;
    prev = searchASL(semAdd);
    if(prev -> s_next -> s_semAdd == semAdd)  /* we found the one we were looking for */
    {
        pcb_PTR p;
        debugC(prev -> s_next -> s_procQ, NULL);
        addokbuf("removing...");
        p = removeProcQ(&prev -> s_next -> s_procQ);
        addokbuf("removed\n");
        debugC(p,NULL);
        if(emptyProcQ(&prev -> s_next -> s_procQ))
        {
            freeSemd(prev -> s_next);
        }
        p -> pcb_semAdd = NULL;
        addokbuf("dick \n");
        return p;
    }
    else
    {
        return NULL;
    }

}

/*
 * same as removeBlocked except call outProcQ instead of removeProcQ
 */
pcb_PTR outBlocked (pcb_PTR p)
{
    semd_PTR q;
    pcb_PTR returnMe;
    int *semAdd;
    semAdd = p -> pcb_semAdd;

    q = searchASL(semAdd);
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == (int*)MAXINT))
    {
        returnMe = NULL;
    }
    else
    {
        if(p ==  NULL)
        {
            returnMe = NULL;
        } 
        else
        {
            returnMe = outProcQ((pcb_PTR*)q -> s_next -> s_procQ, p);
        }   
    }
    return returnMe;
}

/*
 * same as remove/out except call headProcQ and do not deallocate
 */
pcb_PTR headBlocked (int *semAdd)
{
    semd_PTR q;
    q = searchASL(semAdd);
    pcb_PTR returnMe;
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == (int*)MAXINT))
    {
        returnMe = NULL;
    }
    else
    {
        returnMe = headProcQ(q -> s_next -> s_procQ);
    }
    return returnMe;
}

/*
 * create ASL of length MAXPROC
 */
void initASL()
{
    int i;
    semdFree_h = NULL;
    semdActive_h = NULL;
    HIDDEN semd_t semdTable[MAXPROC+2];
    for(i=0;i<MAXPROC+2;i++)
    {
        freeSemd(&semdTable[i]);
    }

    semd_PTR semdZero;
    semd_PTR semdMax;

    semdZero = allocSemd((int*)0);
    semdMax = allocSemd((int*)MAXINT);
    
    semdZero -> s_next = semdMax;

    semdActive_h = semdZero;
}
