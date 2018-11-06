
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

