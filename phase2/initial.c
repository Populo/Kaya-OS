/*****************************************************************
 *                  initial.c                        
 * initial.c contains the main function for the operating system. 
 * This function initializes the pcb free list, ready queue, 
 * and semaphore list. It initializes state locations for each 
 * of the event handler locations: syscalls, pbgtraps, tlbtraps,
 * and interrupts.  It creates a process, sets the pc to call 
 * the p2test function right away, adds it to the readyQueue,
 * and calls scheduler to kick off the operating system.
 * 
 * Writtern by Chris Staudigel and Grant Stapleton
 *****************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/p2test.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "../e/initial.e"

/************************Global Definitions***********************/

int processCount;
int softBlockCount;
pcb_PTR currentProcess;
pcb_PTR readyQueue;
pcb_PTR longReadyQueue;
int sem[TOTALSEM];

/***************************Main Function************************/
/****************************************************************
 * This function performs the work of the file by initializing
 * the global variables, setting the appropriate states for the 
 * handlers, initializing the pcb free list, ready Queue, 
 * and ASL free list, creates a process to run p2test, and adds
 * that process to the ready Queue, then calls scheduler to start
 * the test.
 *****************************************************************/
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
    int temp;
    /* initialize all device semaphores to 0 */
    for(temp = 0; temp < TOTALSEM; temp++)
    {
        sem[temp] = 0;
    }

    /* allocate pcbs and semd freelists */
    initPcbs();
    initASL();

    /* init first process */
    currentProcess = allocPcb();
    /* penultimate page of physical memory */
    currentProcess -> pcb_state.s_sp = (RAMTOP - PAGESIZE);
    /* stack pointer to run test function because we should run the test */
    currentProcess -> pcb_state.s_pc = (memaddr) test;
    /* i dont know what this is but we need to set this too */
    currentProcess -> pcb_state.s_t9 = (memaddr) test;
    /* set the status: All interrupts and local timer on */
    currentProcess -> pcb_state.s_status = ALLOFF | IEON | IMON | LTON;

    /* increment process count */
    ++processCount;

    /* insert new process to ready Queue */
    insertProcQ(&readyQueue, currentProcess);
    currentProcess = NULL;
    LDIT(INTTIME);

    /* call scheduler to get the first process */
    scheduler();

    /* should never reach this */
    return FAILURE;
}

