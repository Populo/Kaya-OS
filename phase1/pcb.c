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

HIDDEN pcb_PTR findLastChild(pcb_PTR p)
{
    if(p -> pcb_nextSib == NULL)
    {
        return p;
    }
    else
    {
        return findLastChild(p -> pcb_nextSib);
    }
}

void debugA(pcb_PTR p) 
{
    int i;
    i = 0;
}

void debugB(pcb_PTR p)
{
    int i;
    i = 0;
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
        p -> pcb_next = p;
        p -> pcb_prev = p;
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
    if(emptyProcQ(*tp))
    {
        returnMe = NULL;
    }    
    else if((*tp)-> pcb_next == (*tp))
    { 
        returnMe = (*tp);
        (*tp) = mkEmptyProcQ();
    }
    else
    {
        returnMe = (*tp) -> pcb_next;
        (*tp) -> pcb_next = (*tp) -> pcb_next -> pcb_next;
        (*tp) -> pcb_next -> pcb_next -> pcb_prev = (*tp);
    }
    

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
        p ->pcb_nextSib = prnt ->pcb_child;
        prnt ->pcb_child = p;
        p ->pcb_prevSib =NULL;  
    }
}

pcb_PTR removeChild (pcb_PTR p)
{
    if(emptyChild(p))
    {
        return NULL;
    }
    else
    {
        if(p -> pcb_child -> pcb_nextSib ==  NULL)
        {
            pcb_PTR temp = p -> pcb_child;
            p -> pcb_child = NULL;
            return temp;
        }
        else
        {
            pcb_PTR temp = p -> pcb_child;
            p -> pcb_child = p-> pcb_child -> pcb_nextSib;
            return temp;
        }
    }
}

pcb_PTR outChild (pcb_PTR p)
{
    if((p == NULL) || (p -> p_prnt == NULL)){
		/* p is not a child */
		return NULL;
	}
	if((p -> p_prnt -> p_child) == p){
		/* am newest child */
		return removeChild(p -> p_prnt);
	}
	if ((p -> p_sibNext) == NULL){
		/* p is at the end of child list */
		p -> p_sibPrev -> p_sibNext = NULL;
		p -> p_prnt = NULL;
		return p;
	}
	if (((p -> p_sibPrev) != NULL) && ((p -> p_sibNext) != NULL)){ 
		/* p is is a middle child */
		p -> p_sibNext -> p_sibPrev = p -> p_sibPrev;
		p -> p_sibPrev -> p_sibNext = p -> p_sibNext;
		p -> p_prnt = NULL;
		return p;
	}
	/* should never get to this */
	return NULL;
}

