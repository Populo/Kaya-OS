#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/initial.e"

#include "../h/types.h"
#include "../h/const.h"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;

cpu_t TODStarted;

void scheduler()
{
    pcb_PTR runningProcess;

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


    pcb_PTR currentProcess;

    currentProcess = removeProcQ(&readyQueue);

    if (currentProcess == NULL)
    {
        PANIC();
    }

    runningProcess = currentProcess;

    LDST(&(runningProcess -> pcb_s));

    
}

