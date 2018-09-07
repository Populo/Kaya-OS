#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"

HIDDEN pcb_PTR *pcb_FREE_h;

HIDDEN pcb_PTR helpOut(pcb_PTR p, pcb_PTR looking)
{
    if(p -> pcb_sibling == looking)
    {
        return p;
    }
    else
    {
       return helpOut(p -> pcb_sibling, looking);
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
    if(p -> pcb_sibling == NULL)
    {
        return p;
    }
    else
    {
        return findLastChild(p -> pcb_sibling);
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

    (*pcb_FREE_h) = mkEmptyProcQ();

    
    for(it = 0; it<MAXPROC; it++)
    {
        freePcb(&(array[it]));
    }
}

void freePcb (pcb_PTR p)
{
    insertProcQ(&pcb_FREE_h, p);
}

pcb_PTR allocPcb ()
{
    return removeProcQ(&pcb_FREE_h);
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
    }
    else
    {
        p -> pcb_parent = prnt;
        pcb_PTR lastChild = findLastChild(prnt-> pcb_child);
        lastChild -> pcb_sibling = p;
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
        if(p -> pcb_child -> pcb_sibling ==  NULL)
        {
            pcb_PTR temp = p -> pcb_child;
            p -> pcb_child = NULL;
            return temp;
        }
        else
        {
            pcb_PTR temp = p -> pcb_child;
            p -> pcb_child = p-> pcb_child -> pcb_sibling;
            return temp;
        }
    }
}

pcb_PTR outChild (pcb_PTR p)
{
    pcb_PTR returnMe;
    if(emptyChild(p))
    {
        returnMe = NULL;
    }
    else
    {
        if(p -> pcb_parent == NULL)
        {
            returnMe = NULL;
        }
        else if(p -> pcb_parent -> pcb_child == p)
        {
            returnMe = p -> pcb_parent -> pcb_child;
            p -> pcb_parent ->  pcb_child = p -> pcb_parent -> pcb_child -> pcb_sibling; 
        }
        else
        {
            pcb_PTR prnt = p -> pcb_parent;           
            pcb_PTR prevSib = helpOut(prnt->pcb_child, p);
            returnMe = prevSib -> pcb_sibling;
            prevSib -> pcb_sibling = p -> pcb_sibling;    
        }
    }
    return returnMe;
}

