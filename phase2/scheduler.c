#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/initial.e"

#include "../h/types.h"
#include "../h/const.h"
#include "/usr/local/include/umps2/umps/libumps.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;

cpu_t TODStarted;

void scheduler()
{

    if (processCount == 0)
    {
        HALT();
    }

    if (emptyProcQ(readyQueue))
    {
        if (softBlockCount == 0)
        {
            PANIC();
        }
        else
        {
            /* wait bit in status register */
            HALT();
        }
    }
    currentProcess = removeProcQ(&readyQueue);
    STCK(TODStarted);
    /*setTimer(QUANTUM);*/
    LDST(&(currentProcess -> pcb_s));

    
    

    if (currentProcess > 0 && softBlockCount == 0)
    {
        PANIC();
    }
    if(processCount > 0 && softBlockCount > 0)
    {
        setSTATUS((getSTATUS() | ALLOFF | IEON | IECON | IMON)); 
        WAIT();
    }

    
    
}

