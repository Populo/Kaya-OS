#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/p2test.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "../e/initial.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int sem[TOTALSEM];

int main()
{
    unsigned int RAMTOP;
    state_PTR stateLocation;
    devregarea_t *device = (devregarea_t *)RAMBASEADDR;

    RAMTOP = (device -> rambase) + (device -> ramsize);

    /* populate 4 new state areas */
    /* new syscall location */
    stateLocation = (state_PTR) SYSCALLNEWAREA;
    stateLocation -> s_sp = RAMTOP;
    stateLocation -> s_pc = stateLocation -> s_t9 = (memaddr) sysCallHandler;
    stateLocation -> s_status = ALLOFF;

    /* new pbg trap location */
    stateLocation = (state_PTR) PBGTRAPNEWAREA;
    stateLocation -> s_sp = RAMTOP;
    stateLocation -> s_pc = stateLocation -> s_t9 = (memaddr) pbgTrapHandler;
    stateLocation -> s_status = ALLOFF;



    /* new tlb management location */
    stateLocation = (state_PTR) TBLMGMTNEWAREA;
    stateLocation -> s_sp = RAMTOP;
    stateLocation -> s_pc = stateLocation -> s_t9 = (memaddr) tlbTrapHandler;
    stateLocation -> s_status = ALLOFF;


    /* new interrupt location */
    stateLocation = (state_PTR) INTPNEWAREA;
    stateLocation -> s_sp = RAMTOP;
    stateLocation -> s_pc = stateLocation -> s_t9 = (memaddr) ioTrapHandler;
    stateLocation -> s_status = ALLOFF;

    /* init globals */
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;
    processCount = 0;
    softBlockCount = 0;

    /* allocate pcbs and semd freelists */
    initPcbs();
    initASL();

    int temp;
    for(temp = 0; temp < TOTALSEM; temp++)
    {
        sem[temp] = 0;
    }

    /* init first process */
    currentProcess = allocPcb();
    /* penultimate page of physical memory */
    currentProcess -> pcb_s.s_sp = (RAMTOP - PAGESIZE);
    /* stack pointer to run test function because we should run the test */
    currentProcess -> pcb_s.s_pc = (memaddr) test;
    /* i dont know what this is but we need to set this too */
    currentProcess -> pcb_s.s_t9 = (memaddr) test;
    /* set the status */
    currentProcess -> pcb_s.s_status = NULL; /* wat */

    ++processCount;

    insertProcQ(&readyQueue, currentProcess);
    currentProcess = NULL;
    LDIT(INTTIME);

    scheduler();

    return -1;
}