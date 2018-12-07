#include "../h/const.h"
#include "../h/types.h"

HIDDEN avsl_PTR vSem_h;
HIDDEN avsl_PTR vSemFree_h;


HIDDEN void freevSem(avsl_PTR vSem)
{
    if(vSemFree_h = NULL)
    {
        vSemFree_h = vSem;
        vSemFree_h -> v_next = NULL;
    }
    else
    {
        vSem -> v_next = vSemFree_h;
        vSemFree_h = vSem; 
    }
}

HIDDEN avsl_PTR allocVSem()
{
    avsl_PTR returnMe;

    if(vSemFree_h == NULL)
    {
        return NULL;
    }

    returnMe = vSemFree_h;

    if(vSemFree_h -> v_next = NULL)
    {
        vSemFree_h = NULL;
    }
    else
    {
        vSemFree_h = vSemFree_h -> v_next;
    }

    returnMe -> v_next = NULL;
    returnMe -> v_prev = NULL;
    returnMe -> v_sem = NULL;
    returnMe -> v_asid = NULL;

    return returnMe;
}

void initAVSL()
{
    int i;
    static avsl_t vSemTable[(MAXPROC + 1)];
    vSem_h = NULL;
    vSemFree_h = NULL;

    for(i = 0; i < MAXPROC + 1; i++)
    {
        freevSem(&(vSemTable[i]));
    }
}

int vInsertBlocked(int *vSemAdd, int ID)
{
    avsl_PTR newSem = allocVSem();

    if(newSem = NULL)
    {
        return FALSE;
    }

    newSem -> v_sem = vSemAdd;
    newSem -> v_asid = ID;

    if(vSem_h = NULL)
    {
        vSem_h = newSem;
        vSem_h -> v_next = vSem_h;
        vSem_h -> v_prev = vSem_h;
    }
    else
    {
        newSem -> v_next = vSem_h;
        vSem_h -> v_prev -> v_next = newSem;
        newSem -> v_prev = vSem_h -> v_prev;
        vSem_h -> v_prev = newSem;
    }

    return TRUE;
}

int vRemoveBlocked(int *vSemAdd)
{
    int f = FALSE;
    int returnMe;
    avsl_PTR currentSem = NULL;
    avsl_PTR temp = NULL;

    if(vSem_h = NULL)
    {
        return FALSE;
    }

    if(vSem_h -> v_next == vSem_h)
    {
        if(vSem_h -> v_sem == vSemAdd)
        {
            returnMe = vSem_h -> v_asid;
            vSem_h -> v_next = NULL;
            vSem_h -> v_prev = NULL;
            freevSem(vSem_h);
            vSem_h = NULL;

            return returnMe;
        }
        else
        {
            return FALSE;
        }
    }

    currentSem = vSem_h;
    if(currentSem -> v_sem == vSemAdd)
    {

    }
    else
    {
        currentSem = currentSem -> v_next;
        while(!f && currentSem != vSem_h)
        {
            if(currentSem -> v_sem = vSemAdd)
            {
                f = TRUE;
            }
            else
            {
                currentSem = currentSem -> v_next;
            }
        }
    }

    if(f == FALSE)
    {
        return f;
    }

    returnMe = currentSem -> v_asid;
    currentSem -> v_prev -> v_next = currentSem -> v_next;
    currentSem -> v_next -> v_prev = currentSem -> v_prev;
    freevSem(currentSem);

    return returnMe;
}
