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
    pcb_PTR newProc = removeProcQ(&(readyQueue));

    if(newProc == NULL)
    {
        debugA(2);
        currentProcess = NULL;

        if(processCount == 0)
        {
            HALT();
        }
        else if(processCount > 0)
        {
            if(softBlockCount == 0)
            {
                debugA(3);
                PANIC();
            }
            if(softBlockCount > 0)
            {
                debugA(3);
                setSTATUS(getSTATUS()| ALLOFF | IEON | IMON);
                WAIT();
            }
        }

    }
    else
    {
        debugA(4);
        currentProcess = newProc;
        STCK(TODStarted);
        setTIMER(QUANTUM);
        debugA(5);
        LDST(&(currentProcess -> pcb_s));
    } 
}

