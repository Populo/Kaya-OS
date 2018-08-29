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
    }   
    else
    {
        p -> pcb_next = (*tp) -> pcb_next;
        (*tp) -> pcb_next = p;
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
        tp = NULL;
    }
    else
    {
        pcb_PTR temp;
        (*tp) -> pcb_next = (*tp) -> pcb_next -> pcb_next;
        temp = (*tp) -> pcb_next;
        (*tp) -> pcb_next = NULL;
        returnMe = temp;
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