/******************************************************************
 * Scheduler.c
 * 
 * This file handles the queue of ready to run processes and also
 * handles the occasion that there are no process to run whether
 * that means there are no running processes or they are all
 * blocked.
 * 
 * Written by Chris Staudigel and Grant Stapleton
 *****************************************************************/

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
extern pcb_PTR longReadyQueue;

extern void loadAllOfTheStates(state_PTR state);

HIDDEN pcb_PTR getANewJob();

cpu_t currentTOD;
cpu_t TODStarted;


/******************************************************************
 * The scheduler function takes a job from the ready queue and 
 * starts the job.  If there are no ready jobs, the scheduler will
 * either halt if we are out of jobs to run, panic if there is an 
 * inconsistency, or wait for an interrupt if all jobs are blocked.
 *****************************************************************/
void scheduler()
{
    if(currentProcess != NULL)
    {
        STCK(currentTOD);
        currentProcess -> pcb_time = (currentProcess -> pcb_time) + (currentTOD - TODStarted);
    }
    /* grab the new job */
    pcb_PTR newProc = getANewJob();

    /* we did not receive a job to run */
    if(newProc == NULL)
    {
        currentProcess = NULL;

        /* we are out of jobs to run */
        if(processCount == 0)
        {
            HALT();
        }
        /* we are not out of jobs to run */
        else if(processCount > 0)
        {
            /* jobs are not soft blocked (Error) */
            if(softBlockCount == 0)
            {
                PANIC();
            }
            /* all jobs are blocked */
            if(softBlockCount > 0)
            {
                /* turn on interrupts and wait for one to happen */
                setSTATUS(getSTATUS() | ALLOFF | IEON | IECON);
                WAIT();
            }
        }

    }
    else /* We received a job to run */
    {
        /* set current process */
        currentProcess = newProc;
        /* store the current time */
        STCK(TODStarted);
        /* start a timer for the job */
        setTIMER(QUANTUM);
        /* Context Switch - load the process */
        loadAllOfTheStates(&(newProc -> pcb_state));
    } 
}


HIDDEN pcb_PTR getANewJob()
{
    return emptyProcQ(&longReadyQueue) 
        ? removeProcQ(&readyQueue) 
            : removeProcQ(&longReadyQueue);
}
