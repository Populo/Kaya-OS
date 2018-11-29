
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

