#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"

HIDDEN pcb_PTR pcb_FREE_h;

HIDDEN pcb_PTR helpOut(pcb_PTR p, pcb_PTR looking)
{
    if(p -> pcb_nextSib == looking)
    {
        return p;
    }
    else
    {
       return helpOut(p -> pcb_nextSib, looking);
    }
}

HIDDEN pcb_PTR findP(pcb_PTR check, pcb_PTR find, pcb_PTR tail)
{
    if (check == find) 
    {
        return find;
    }
    else if (check == tail) 
    {
        return NULL;
    } 
    else
    {
        return findP(check->pcb_next, find, tail);
    }
}

pcb_PTR mkEmptyProcQ ()
{
    return NULL;
}

int emptyProcQ (pcb_PTR tp)
{
    return (tp == NULL);
}

void initPcbs ()
{
    int it;
    HIDDEN pcb_t array[MAXPROC];

    pcb_FREE_h = NULL;

    
    
    for(it = 0; it<MAXPROC; it++)
    {
        freePcb(&array[it]);
    }
}

void freePcb (pcb_PTR p)
{
    p -> pcb_next = pcb_FREE_h;
    pcb_FREE_h = p; 
}

pcb_PTR allocPcb ()
{
    pcb_PTR returnME;
    if (pcb_FREE_h == NULL)
    {
        returnME = NULL;
    }
    else
    {
        returnME = pcb_FREE_h;
        pcb_FREE_h = pcb_FREE_h -> pcb_next;

        /* break references to queue */
        returnME -> pcb_next = NULL;
        returnME -> pcb_prev = NULL;

        /* break references to children */
        returnME -> pcb_child = NULL;
        returnME -> pcb_prevSib = NULL;
        returnME -> pcb_nextSib = NULL;

        /* semaphore values to null */
        returnME -> pcb_semAdd = NULL;
    }

    return returnME;
}

void insertProcQ (pcb_PTR *tp, pcb_PTR p)
{
    if(emptyProcQ(*tp))
    {
        p -> pcb_prev = p;
        p -> pcb_next = p;       
    } 
    else
    {
        p -> pcb_prev = (*tp);
        p -> pcb_next = (*tp) -> pcb_next;
        (*tp) -> pcb_next = p;
        p -> pcb_next -> pcb_prev = p; 
    }
    *tp = p;
}
pcb_PTR removeProcQ (pcb_PTR *tp)
{
    pcb_PTR returnMe;

    returnMe = outProcQ(tp, headProcQ(*tp));

    return returnMe;
}
pcb_PTR headProcQ (pcb_PTR tp)
{
    if(emptyProcQ(tp))
    {
        return NULL;
    }
    else
    {
        return tp -> pcb_next;
    }
}

pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p) 
{
    pcb_PTR returnMe;

    if (!emptyProcQ(*tp)) 
    {
        if ((*tp) == p) 
        {
            if((*tp)->pcb_next == (*tp))
            {
                returnMe = (*tp);
                (*tp) = mkEmptyProcQ();
            }
            else
            {
                (*tp) -> pcb_prev -> pcb_next = (*tp) -> pcb_next;
                (*tp) -> pcb_next -> pcb_prev = (*tp) -> pcb_prev;
                *tp = (*tp) -> pcb_prev;
            }
            returnMe = p;
        }
        else 
        {
            pcb_PTR foundP = findP((*tp)->pcb_next, p, (*tp));

            if (foundP != NULL) 
            {
                foundP -> pcb_prev -> pcb_next = foundP -> pcb_next;
                foundP -> pcb_next -> pcb_prev = foundP -> pcb_prev;
                returnMe = foundP;
            }
            else
            {
                returnMe = NULL;
            }
        }
    }
    else
    {
        returnMe = NULL;
    }
    
    return returnMe;
}



int emptyChild (pcb_PTR p)
{
    return (p -> pcb_child == NULL);
}

void insertChild (pcb_PTR prnt, pcb_PTR p)
{
    if (!emptyProcQ(prnt)) {
        if(emptyChild(prnt))
        {
            prnt -> pcb_child = p;
            p -> pcb_parent = prnt;
            p-> pcb_nextSib = NULL;
            p -> pcb_prevSib = NULL;
        }
        else
        {
            prnt -> pcb_child -> pcb_prevSib = p;
            p -> pcb_nextSib = prnt ->pcb_child;
            p -> pcb_parent = prnt;
            prnt -> pcb_child = p;
            p -> pcb_prevSib = NULL;  
        }
    }
   
}

/*
 * remove first child in stack
 */
pcb_PTR removeChild (pcb_PTR parent)
{
    return outChild(parent->pcb_child);
}

/*
 * remove given child from anywhere in the stack
 */
pcb_PTR outChild (pcb_PTR child)
{
    /* 5 cases:
     *      not a child/null
     *      only child
     *      first child
     *      middle child
     *      last child
     */

    pcb_PTR returnMe;
    if ((child == NULL) || (child -> pcb_parent == NULL)) /* not a child/null */
    {
        
        returnMe = NULL;
    }
    else if ((child -> pcb_nextSib == NULL) && (child -> pcb_prevSib == NULL) && (child == (child -> pcb_parent -> pcb_child))) /* only child */
    {        
        returnMe = child;  
        child -> pcb_parent -> pcb_child = NULL;
        child -> pcb_parent = NULL;
    }
    else if (child == (child -> pcb_parent -> pcb_child)) /* first child */
    {   
        child -> pcb_nextSib -> pcb_prevSib = NULL;
        child -> pcb_parent -> pcb_child = child -> pcb_nextSib;
        child -> pcb_parent = NULL;
        returnMe = child;
    }
    else if ((child -> pcb_nextSib != NULL) && (child -> pcb_prevSib != NULL)) /* middle child */
    {
        child -> pcb_prevSib -> pcb_nextSib = child -> pcb_nextSib;
        child -> pcb_nextSib -> pcb_prevSib = child -> pcb_prevSib;
        child -> pcb_parent = NULL;
        returnMe = child;
    }
    else if ((child -> pcb_nextSib == NULL) && (child -> pcb_prevSib != NULL)) /* last child */
    {
        child -> pcb_prevSib -> pcb_nextSib = NULL;
        child -> pcb_parent = NULL;
        returnMe = child;
    }
    else
    {
        returnMe = NULL;
    }

    return returnMe;
} 

