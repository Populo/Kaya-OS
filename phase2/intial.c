#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"

#include "../e/initial.e"

HIDDEN int processCount;
HIDDEN softBlockCount;
HIDDEN pcb_PTR currentProcess;
HIDDEN pcb_PTR readyQueue;


int main()
{
    /* populate 4 new state areas */
    state_PTR newLocation;

    /* new syscall location */
    newLocation = (state_PTR) SYSCALLNEWAREA;
    

    /* new pbg trap location */
    newLocation = (state_PTR) PBGTRAPNEWAREA;

    /* new table management location */
    newLocation = (state_PTR) TBLMGMTNEWAREA;

    /* new interrupt location */
    newLocation = (state_PTR) INTPNEWAREA;
    return 0;
}