
#include "../h/types.h"
#include "../h/const.h"

#include "../e/adl.e"

HIDDEN adl_PTR delaydFree_h;
HIDDEN adl_PTR activeDelayd_h;

/* search delaydFree for node before address we are looking for */
HIDDEN adl_PTR searchDelayd(int *wake);

adl_PTR searchDelayd(int *wake)
{
    adl_PTR searching = activeDelayd_h;

    while (searching -> d_next != NULL &&
            searching -> d_next -> d_wakeTime < wake)
            {
                searching = searching -> d_next;
            }
    
    return searching;
}

void initADL()
{
    int i;
    static adl_t delayTable[(MAXPROC + 1)];

    delaydFree_h = NULL;
    activeDelayd_h = NULL;

    for(i =0; i < MAXPROC+1; i++)
    {
        freeDelayd(&(delayTable[i]));
    }
}

int headDelayTime()
{
    if(activeDelayd_h == NULL)
    {
        return FAILURE;
    }
    return activeDelayd_h -> d_wakeTime;
}

HIDDEN void freeDelayd(adl_PTR delay)
{
    if(delaydFree_h == NULL)
    {
        delaydFree_h = delay;
        delaydFree_h = NULL;
    }
    else{
        delay -> d_next = delaydFree_h;
        delaydFree_h = delay;
    }
}

HIDDEN adl_PTR allocDelayd()
{
    adl_PTR returnMe;

    if(delaydFree_h = NULL)
    {
        return NULL;
    }
    else
    {
        returnMe = delaydFree_h;
        if(returnMe -> d_next == NULL)
        {
            delaydFree_h = NULL;
        }
        else
        {
            delaydFree_h = delaydFree_h -> d_next;
        }
        returnMe -> d_next = NULL;
    }
    return returnMe;
}

int insertDelay(int wakeTime, int ID)
{
    adl_PTR delay = NULL;

    adl_PTR new = allocDelayd();

    if(new == NULL)
    {
        return FALSE;
    }

    new -> d_wakeTime = wakeTime;
    new -> d_asid = ID;

    if(activeDelayd_h == NULL)
    {
        activeDelayd_h = new;
        new -> d_next = NULL;

        return TRUE;
    }

    delay = searchDelayd(wakeTime);

    new -> d_next = delay -> d_next;
    delay -> d_next = new;

    return TRUE;
}

int removeDelay()
{
    int returnMe;

    adl_PTR temp = activeDelayd_h;

    if(temp != NULL)
    {
        activeDelayd_h = temp -> d_next;

        returnMe = temp -> d_asid;
        temp -> d_next = NULL;
        freeDelayd(temp);

        return returnMe;
    }

    return FAILURE;
}
