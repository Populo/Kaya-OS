#include "../h/types.h"
#include "../h/const.h"

pcb_PTR mkEmptyProcQ ()
{
    return NULL;
}
int emptyProcQ (pcb_PTR tp)
{
    return tp == NULL;
}
void insertProcQ (pcb_PTR *tp, pcb_PTR p)
{
    if(emptyProcQ(tp))
    {
        p -> pcb_next = p;
        p -> pcb_prev = p;
    }   
    else
    {
        p -> pcb_prev = tp;
        p -> pcb_next = (*tp) -> pcb_next;
        (*tp) -> pcb_next = p;
        p -> pcb_next -> pcb_prev = p;
    }
    
    *tp = p;
}
pcb_PTR removeProcQ (pcb_PTR *tp)
{
    pcb_PTR returnMe;
    if(emptyProcQ(tp))
    {
        returnMe = NULL;
    }    
    else if((*tp)-> pcb_next == (*tp))
    { 
        (*tp) -> pcb_next = tp;
        returnMe = tp;
    }
    else
    {
        pcb_PTR temp;
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

    if (!emptyProcQ(tp)) 
    {
        if (tp == p) 
        {
            // delete p
            // move tp
            returnMe = p;
        }
        else 
        {
            pcb_PTR foundP = findP((*tp)->pcb_next, p, tp);

            if (foundP != NULL) 
            {
                // delete p
                returnMe = foundP;
            }
            else
            {
                return NULL;
            }
        }
    }
}

static pcb_PTR findP(pcb_PTR check, pcb_PTR find, pcb_PTR tail)
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

int emptyChild (pcb_PTR p)
{

}
void insertChild (pcb_PTR prnt, pcb_PTR p)
{

}
pcb_PTR removeChild (pcb_PTR p)
{

}
pcb_PTR outChild (pcb_PTR p)
{

}