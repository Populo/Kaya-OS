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

HIDDEN semd_PTR searchASL(int *semAdd, semd_PTR s);

HIDDEN semd_PTR allocSemd(int *semAdd)
{
    addokbuf("n");
    semd_PTR returnMe;
    if(semdFree_h -> s_next == NULL)
    {
        addokbuf("1");
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
            if (semAdd == (int*) MAXINT)
            {
                semd_PTR max;
                max -> s_semAdd = semAdd;

                returnMe -> s_next = max -> s_next;
                max -> s_next = returnMe;
            }
            else
            {
                semd_PTR prev;
                prev = searchASL(semAdd, semd_h);

                returnMe -> s_next = prev -> s_next;
                prev -> s_next = returnMe;
            }
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
    if( s -> s_next-> s_semAdd < semAdd)
    {
        return s -> s_next;
    }
    else if(semAdd == NULL)
    {
        semAdd = (int*) MAXINT;
        return searchASL(semAdd, s);
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
        p -> pcb_semAdd = semAdd;
        insertProcQ(&q -> s_next, p);

    }
    else
    {
        if(semdFree_h != NULL)
        {
            semd_PTR temp;
            semd_PTR new;
            temp = q -> s_next;
            new = allocSemd(semAdd);
            if(new == NULL)
            {
                return TRUE;
            }
            else
            {
                q -> s_next = new;
                new -> s_next = temp;
                new -> s_procQ = mkEmptyProcQ();
                new -> s_semAdd = semAdd;
                insertProcQ(&new -> s_procQ, p);
            }        
        }
        else
        {
            return TRUE;
        }
    }
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
    semd_PTR q;
    q = searchASL(semAdd, semd_h);
    if(q -> s_next -> s_semAdd != semAdd)  /* ||(q -> s_next -> s_semAdd == (int*) MAXINT) */
    {
        addokbuf("/");
        return NULL;
    }
    else
    {
        q = q -> s_next;
        pcb_PTR p;
        p = removeProcQ((pcb_PTR*)q->s_procQ);
        if(p == NULL)
        {
            addokbuf("*");
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
    semdFree_h = NULL;
    HIDDEN semd_t semdTable[MAXPROC+2];
    for(i=0;i<MAXPROC+2;i++)
    {
        
        freeSemd(&semdTable[i]);
    }

    allocPcb((int*)0);
    allocPcb((int*)MAXINT);
    
}
