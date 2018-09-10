#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"

#include "../e/asl.e"

semd_PTR semdFree_h, /* Head of free list */
         semd_h;     /* Head of ASL */


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
HIDDEN semd_PTR searchASL(int *semAdd, semd_PTR s)
{
    if(s -> s_next -> s_semAdd == semAdd)
    {
        return s;
    }
    else if(&(s -> s_next -> s_semAdd) == MAXINT)
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
   if(q -> s_next -> s_semAdd == semAdd)
   {
        insertProcQ(*(q -> s_next -> s_procQ), p);

   }
   else
   {
       if(semdFree_h != NULL)
       {
            semd_PTR temp;
            temp -> s_next = q -> s_next;
            q -> s_next = allocSemd(semAdd);
            q -> s_next -> s_next = temp;
            q -> s_next -> s_procQ = mkEmptyProcQ();
            q -> s_next -> s_semAdd = semAdd;
            insertProcQ(*(q -> s_next -> s_procQ), p);
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
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == MAXINT))
    {
        return NULL;
    }
    else
    {
        q = q -> s_next;
        pcb_PTR p;
        p = removeProcQ(q->s_procQ);
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
    q = searchASL(p -> pcb_semAdd, semd_h);
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == MAXINT))
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
            returnMe = outProcQ(q -> s_next -> s_procQ, p);
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
    if((q -> s_next -> s_semAdd != semAdd)||(q -> s_next -> s_semAdd == MAXINT))
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
    HIDDEN semd_t semdTable[MAXPROC];
    for(i=0;i<MAXPROC+2;i++)
    {
        if(i==0)
        {
            semd_PTR s;
            s -> s_semAdd = 0;
            freeSemd(s);
        }
        else if(i == MAXPROC+1)
        {
            semd_PTR s;
            s -> s_semAdd = MAXINT;
            freeSemd(s);
        }
        else
        {
            freeSemd(&semdTable[i]);
        }
    }
}