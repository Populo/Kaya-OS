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

cpu_t currentTOD;
cpu_t TODStarted;


void debugA(int i)
{
    int temp;
    temp = i;
}

void scheduler()
{
    debugA(1);
    if(currentProcess != NULL)
    {
        debugA(100);
        STCK(currentTOD);
        currentProcess -> pcb_time = (currentProcess -> pcb_time) + (currentTOD - TODStarted);
        debugA(110);
    }
    if(emptyProcQ(readyQueue))
    {
        currentProcess = NULL;
        debugA(2);
        if(processCount == 0)
        {
            HALT();
        }

        if(processCount > 0 && softBlockCount == 0)
        {
            PANIC();
        }

        if(processCount > 0 && softBlockCount > 0)
        {
            debugA(3);
            setSTATUS(getSTATUS()| ALLOFF | IEON | IMON);
            debugA(4);
            WAIT();
        }
    }
    else
    {
        debugA(5);
        currentProcess = removeProcQ(&readyQueue);
        STCK(TODStarted);
        setTIMER(QUANTUM);
        debugA(6);
        LDST(&(currentProcess -> pcb_s));

    }
    
}

