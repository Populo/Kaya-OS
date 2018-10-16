/*******************************pcb.c**************************************/
/*            Queue manager for the Kaya Operating System.                */
/*            The methods in this file will create, delete                */
/*             and otherwise manage queues of type pcb_t                  */
/*              By: Chris Staudigel and Grant Stapleton                   */
/*              with starting code by Michael Goldweber                   */
/**************************************************************************/

#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"

HIDDEN pcb_PTR pcb_FREE_h;

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
 * free list implemented as a stack
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
    pcb_PTR newPcb;

    /* check for an available free pcb on the free list */
    if (pcb_FREE_h == NULL)
    {
        /* there isnt a free pcb, error */
        return NULL;
    }

    /* using first pcb on free list */
    newPcb = pcb_FREE_h;
    pcb_FREE_h = pcb_FREE_h -> pcb_next;

    /* break references to queue */
    newPcb -> pcb_next = NULL;
    newPcb -> pcb_prev = NULL;

    /* break references to children */
    newPcb -> pcb_child = NULL;
    newPcb -> pcb_prevSib = NULL;
    newPcb -> pcb_nextSib = NULL;

    /* semaphore values to null */
    newPcb -> pcb_semAdd = NULL;

    /* PULL UP AND DIE MF */
    int i;
    for (i = 0; i < SECTIONS; i++)
    {
        newPcb -> pcb_states[i][OLD] = NULL;
        newPcb -> pcb_states[i][NEW] = NULL;
    }


    return newPcb;
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
    
    /* new tail is p */
    *tp = p;
}

/*
 * dequeue operation on queue pointed to by tp
 */
pcb_PTR removeProcQ (pcb_PTR *tp)
{
    /* call outproc with tp and head of the queue */
    return outProcQ(tp, headProcQ(*tp));
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

    /* error case: queue is empty */
    if (emptyProcQ(*tp))
    {
        return NULL;
    }

    /* if we are looking for tp */
    if ((*tp) == p) 
    {
        /* if tp is the only element in the queue */
        if((*tp)->pcb_next == (*tp))
        {
            (*tp) = mkEmptyProcQ();
        }
        else
        {
            /* remove tp from the queue gracefully */
            (*tp) -> pcb_prev -> pcb_next = (*tp) -> pcb_next;
            (*tp) -> pcb_next -> pcb_prev = (*tp) -> pcb_prev;
            *tp = (*tp) -> pcb_prev;
        }

        return p;
    }

    /* 
     * we are not looking for tp, so search for p in the given queue
     * checking head first because loop breaks if checking == tp 
     */
    pcb_PTR searching;
    
    searching = headProcQ(*tp);

    while (searching != *tp && searching != p)
    {
        searching = searching -> pcb_next;
    }

    /* we did not find p on the stack */
    if (searching == *tp)
    {
        return NULL;
    }

    /* we found the pcb on the queue */
    p -> pcb_prev -> pcb_next = p -> pcb_next;
    p -> pcb_next -> pcb_prev = p -> pcb_prev;
    return p;
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
    /* there are no children */
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

     /* not a child/null */
    if ((child == NULL) || (child -> pcb_parent == NULL))
    {
        return NULL;
    }

     /* only child */
    if ((child -> pcb_nextSib == NULL) && (child -> pcb_prevSib == NULL) && (child == (child -> pcb_parent -> pcb_child)))
    {          
        child -> pcb_parent -> pcb_child = NULL;
        child -> pcb_parent = NULL;

        return child;
    }

     /* first child */
    if (child == (child -> pcb_parent -> pcb_child))
    {   
        child -> pcb_nextSib -> pcb_prevSib = NULL;
        child -> pcb_parent -> pcb_child = child -> pcb_nextSib;
        child -> pcb_parent = NULL;
        
        return child;
    }

     /* middle child */
    if ((child -> pcb_nextSib != NULL) && (child -> pcb_prevSib != NULL))
    {
        child -> pcb_prevSib -> pcb_nextSib = child -> pcb_nextSib;
        child -> pcb_nextSib -> pcb_prevSib = child -> pcb_prevSib;
        child -> pcb_parent = NULL;
        
        return child;
    }

     /* last child */
    if ((child -> pcb_nextSib == NULL) && (child -> pcb_prevSib != NULL))
    {
        child -> pcb_prevSib -> pcb_nextSib = NULL;
        child -> pcb_parent = NULL;
        return child;
    }

    /* error case */
    return NULL;

} 

