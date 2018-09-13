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
        returnMe -> s_next = NULL;
        returnMe -> s_procQ = NULL;
        returnMe -> s_semAdd = semAdd;
    }

    return returnMe;
}

HIDDEN void freeSemd(semd_PTR s) 
{
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
    while (searching -> s_next -> s_semAdd < semAdd && searching -> s_next != NULL)
    {
        debugA(semAdd, searching -> s_next -> s_semAdd);
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
        insertProcQ(&q -> s_next -> s_procQ, p);
        return FALSE;
    }
    else
    {
        q = allocSemd(semAdd);
        if (q == NULL)
        {
            return TRUE;
        }
        else
        {
            p -> pcb_semAdd = semAdd;
            insertProcQ(&q -> s_procQ, p);
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
    semd_PTR q;
    q = searchASL(semAdd);
    if(q -> s_semAdd == semAdd)  /* (q -> s_next -> s_semAdd == (int*) MAXINT) */
    {
        semd_PTR temp;
        temp = q -> s_next;
        pcb_PTR p;
        p = removeProcQ(&(q->s_procQ));
        if(p == NULL)
        {
            addokbuf("*");
            temp = q -> s_next;
            q -> s_next = q -> s_next -> s_next;
            freeSemd(q);
        }
        return p;
    }
    else
    {
        addokbuf("/");
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
    addokbuf("who?  \n");

    semd_PTR semdZero, semdMax;

    semdZero = allocPcb(0);
    semdMax = allocPcb(MAXINT);
    
    semdZero -> s_next = semdMax;

    semdActive_h = semdZero;

    addokbuf((char*) semdActive_h);
    addokbuf("\n");
}
