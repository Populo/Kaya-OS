#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"

HIDDEN pcb_PTR pcb_FREE_h;

/* ================ Helper Methods ================== */

/*
 * method to loop through a queue and return the found pcb if found or null
 * 
 * params
 * check -> current procQ we are checking
 * find -> procQ we are looking for
 * tail -> tail pointer of queue
 */
HIDDEN pcb_PTR findP(pcb_PTR check, pcb_PTR find, pcb_PTR tail)
{
    /* loop through if we havent found pcb and we havent made it around the queue */
    while(check != tail && find != check)
    {
        /* check next element in queue */
        check = check -> pcb_next;
    }
    /* we did not find the pcb in the queue */
    if(check == tail)
    {
        check = NULL;
    }

    return check;

}

/* ================================= queue management ========================== */

/*
 * Create an empty procQ
 */
pcb_PTR mkEmptyProcQ ()
{
    /* we dont need dummy nodes */
    return NULL;
}

/*
 * Return TRUE if the queue whose tail is pointed to bytpis empty.Return FALSE otherwise. 
 */
int emptyProcQ (pcb_PTR tp)
{
    return (tp == NULL);
}

/*
 * initialize MAXPROC amount of pcbs and store them on the free list
 */
void initPcbs ()
{
    int it;
    /* static array of length MAXPROC of pcbs */
    HIDDEN pcb_t array[MAXPROC];

    /* clean free list */
    pcb_FREE_h = NULL;
    
    for(it = 0; it<MAXPROC; it++)
    {
        /* for each pcb in array, free it */
        freePcb(&array[it]);
    }
}

/*
 * add given pcb pointer to the free list
 */
void freePcb (pcb_PTR p)
{
    p -> pcb_next = pcb_FREE_h;
    pcb_FREE_h = p; 
}

/*
 * allocate a pcb from the free list
 */
pcb_PTR allocPcb ()
{
    pcb_PTR returnME;
    /* check for an available free pcb on the free list */
    if (pcb_FREE_h == NULL)
    {
        /* there isnt a free pcb */
        returnME = NULL;
    }
    else
    {
        /* using first pcb on free list */
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

/*
 * inserts a given pcb onto the queue referenced by tp
 */
void insertProcQ (pcb_PTR *tp, pcb_PTR p)
{
    /* if tp is empty */
    if(emptyProcQ(*tp))
    {
        /* create circularly linked queue with p */
        p -> pcb_prev = p;
        p -> pcb_next = p;     
    }
    else
    {
        /* push tp up queue and replace it with p */
        p -> pcb_prev = (*tp);
        p -> pcb_next = (*tp) -> pcb_next;
        (*tp) -> pcb_next = p;
        p -> pcb_next -> pcb_prev = p; 
    }
    
    *tp = p;
}

/*
 * dequeue operation on queue pointed to by tp
 */
pcb_PTR removeProcQ (pcb_PTR *tp)
{
    pcb_PTR returnMe;

    /* call outproc with tp and head of the queue */
    returnMe = outProcQ(tp, headProcQ(*tp));

    return returnMe;
}

/*
 * returns the head of the queue without removing it from the queue
 */
pcb_PTR headProcQ (pcb_PTR tp)
{
    /* nonexistant queue */
    if(emptyProcQ(tp))
    {
        return NULL;
    }
    else
    {
        /* return head of queue */
        return tp -> pcb_next;
    }
}

/*
 * look for p in the queue referenced by tp
 * if its found remove it from the queue
 * if not return null
 */
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p) 
{
    pcb_PTR returnMe;
    /* check for empty */
    if (!emptyProcQ(*tp)) 
    {
        /* if we are looking for tp */
        if ((*tp) == p) 
        {
            /* if tp is the only element in the queue */
            if((*tp)->pcb_next == (*tp))
            {
                /* make the queue empty */
                returnMe = (*tp);
                (*tp) = mkEmptyProcQ();
            }
            else
            {
                /* remove tp from the queue gracefully */
                (*tp) -> pcb_prev -> pcb_next = (*tp) -> pcb_next;
                (*tp) -> pcb_next -> pcb_prev = (*tp) -> pcb_prev;
                *tp = (*tp) -> pcb_prev;
            }
            returnMe = p;
        }
        else 
        {
            /* search for p in the given queue
             * checking head first because loop breaks if checking == tp 
             */
            pcb_PTR foundP = findP((*tp) -> pcb_next, p, (*tp));
            
            if (foundP != NULL)
            {
                /* we found the pcb on the queue */
                foundP -> pcb_prev -> pcb_next = foundP -> pcb_next;
                foundP -> pcb_next -> pcb_prev = foundP -> pcb_prev;
                returnMe = foundP;
            }
            else
            {
                /* we did not find the pcb */
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

/* =============================== Child Management ========================== */

/*
 * check for an empty child list
 */
int emptyChild (pcb_PTR p)
{
    /* again, dummy nodes are for nerds */
    return (p -> pcb_child == NULL);
}

/*
 * insert a new child onto the list of children of prnt
 */
void insertChild (pcb_PTR prnt, pcb_PTR p)
{
    /* if the parent exists */
    if (!emptyProcQ(prnt)) {
        /* if there are no children */
        if(emptyChild(prnt))
        {
            /* create a new child list on parent */
            prnt -> pcb_child = p;
            p -> pcb_parent = prnt;
            p-> pcb_nextSib = NULL;
            p -> pcb_prevSib = NULL;
        }
        else
        {
            /* insert child into family */
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

