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


int debugA(int i)
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
        currentProcess -> cpu_time = (currentProcess -> cpu_time) + (currentTOD - TODStarted);
        debugA(110);
    }
    if (processCount == 0)
    {
        debugA(200);
        HALT();
    }

    debugA(2);
    if (emptyProcQ(readyQueue))
    {
        debugA(300);
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
  /*   if (processCount > 0 && softBlockCount == 0)
    {
        PANIC();
    }
    if(processCount > 0 && softBlockCount > 0)
    {
        setSTATUS((getSTATUS() | ALLOFF | IEON | IECON | IMON)); 
        WAIT();
    } */
    debugA(3);
    currentProcess = removeProcQ(&readyQueue);
    --processCount;
    debugA(4);
    STCK(TODStarted);
    debugA(15);
    setTimer(QUANTUM);
    debugA(16);
    LDST(&(currentProcess -> pcb_s));

    debugA(5);
    

    

    debugA(6);
    
}

