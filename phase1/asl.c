#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"

#include "../e/asl.e"


semd_PTR semdFree_h, /* Head of free list */
         semd_h;     /* Head of ASL */

void debugA(int a)
{
    int i;
    i = 1;
}


HIDDEN semd_PTR allocSemd(int *semAdd)
{
    semd_PTR returnMe;
    if(semdFree_h == NULL)
    {
        returnMe = NULL;
    }
    else
    {
        returnMe = semdFree_h;
        semdFree_h = semdFree_h -> s_next;

        returnMe -> s_next = NULL;
        returnMe -> s_procQ = mkEmptyProcQ();
        returnMe -> s_semAdd = semAdd;

        if (semd_h == NULL)
        {
            semd_h = returnMe;
        }
        else
        {
            semd_PTR prev;
            prev = searchASL(semAdd, semd_h);

            returnMe -> s_next = prev -> s_next;
            prev -> s_next = returnMe;
        }
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
HIDDEN semd_PTR searchASL(int *semAdd, semd_PTR s)
{
    if((s -> s_semAdd < semAdd) && (s -> s_next -> s_semAdd >= semAdd))
    {
        return s;
    }
    else if(s -> s_next -> s_semAdd == (int*) MAXINT)
    {
        return s;
    }
    else
    {
       return searchASL(semAdd, s->s_next);
    }
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
    q = searchASL(semAdd, semd_h);
    pcb_PTR lol;
    lol = q -> s_next -> s_procQ;

    if(q -> s_next -> s_semAdd == semAdd)
    {
        p -> pcb_semAdd = semAdd;
        insertProcQ((pcb_PTR*) lol, p);

    }
    else
    {
        if(semdFree_h != NULL)
        {
            semd_PTR temp;
            temp -> s_next = q -> s_next;
            q -> s_next = allocSemd(semAdd);
            q -> s_next -> s_next = temp;
            lol = mkEmptyProcQ();
            q -> s_next -> s_semAdd = semAdd;
            insertProcQ((pcb_PTR*) lol, p);
        }
        else
        {
            return 1;
        }
    }
    return 0;
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
    q = searchASL(semAdd, semd_h);
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == (int*) MAXINT))
    {
        return NULL;
    }
    else
    {
        q = q -> s_next;
        pcb_PTR p;
        p = removeProcQ((pcb_PTR*)q->s_procQ);
        if(p == NULL)
        {
            freeSemd(q);
            return NULL;
        }
        else
        {
            return p;
        }
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

    q = searchASL(semAdd, semd_h);
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
    q = searchASL(semAdd, semd_h);
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
    HIDDEN semd_t semdTable[MAXPROC + 2];
    for(i=0;i<MAXPROC;i++)
    {
        
        freeSemd(&semdTable[i]);
    }
   
    semd_PTR dummyZero, dummyMax;

    dummyZero -> s_semAdd = (int*) 0;
    dummyMax -> s_semAdd = (int*) MAXINT;

    semd_h = dummyZero;
    dummyZero -> s_next = dummyMax;

}
