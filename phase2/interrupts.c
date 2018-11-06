/******************************************************************
 * interrupts.c
 * 
 * This file holds the interrupt handler which is called along 
 * with sys8 to handle the io interrupt.  The handler will 
 * determine the interrupt line, and from the line the specific
 * device that caused the interrupt in order to properly handle
 * that interrupt.
 * 
 * Written by Chris Staudigel and Grant Stapleton
 *****************************************************************/
#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* globals from initial.c */
extern cpu_t TODStarted;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int sem[TOTALSEM];

cpu_t TODStopped;

/* copy state method from exceptions */
extern void copyState(state_PTR old, state_PTR new);

/* hidden helper methods */
HIDDEN void finish();
HIDDEN int getDeviceNumber(int lineNumber);

/******************************************************************
 * ioTrapHandler
 * 
 * This handler will first determine the interrupt line then from
 * there determine the specific device.  From there it will perform
 * a V operation on the specified device semaphore to handle the 
 * interrupt. 
 * 
 * If a process throws an interrupt we do not return control to
 * that process, we place it back on the ready queue and call 
 * scheduler for a new job.
 *****************************************************************/
void ioTrapHandler()
{
    cpu_t end;
    int deviceNum, interruptNum;
    device_t* devRegNum;
    int i, status, tranStatus;
    int* semAdd;
    pcb_PTR temp;
    state_PTR old = (state_PTR) INTPOLDAREA;

    /* store time interrupt happens */
    STCK(TODStopped);

    /* determine which line threw the interrupt */
    /* this interrupt should never happen */
    if((old -> s_cause & LINEZERO) == LINEZERO)
    {
        PANIC();
    }
    /* this interrupt is called when the process runs out of time */
    else if((old -> s_cause & LINEONE) == LINEONE)
    {
        finish();
    }
    /* interval timer, unblock all processes blocked on clock */
    else if((old -> s_cause & LINETWO) == LINETWO)
    {
        LDIT(INTTIME); /* load 100 ms into timer */
        semAdd = (int*) &(sem[TOTALSEM-1]);
        while(headBlocked(semAdd) != NULL)
        {
            temp = removeBlocked(semAdd);
            STCK(end);
            if(temp != NULL)
            {
                /* insert job onto ready Queue */
                insertProcQ(&readyQueue, temp);
                /* bill process the time */
                (temp -> pcb_time) = (temp -> pcb_time) + (end - TODStopped);
                softBlockCount--;
            }
        }
        /* set semaphore to zero to reset it fully */
        (*semAdd) = 0;
        /* finish the interrupt */
        finish();
    }  
    /* disk device */
    else if((old -> s_cause & LINETHREE) == LINETHREE)
    {
        interruptNum = DISKINT;
    }
    /* tape device */
    else if((old -> s_cause & LINEFOUR) == LINEFOUR)
    {
        interruptNum = TAPEINT;
    }
    /* network device */
    else if((old -> s_cause & LINEFIVE) == LINEFIVE)
    {
        interruptNum = NETWINT;
    }
    /* printer device */
    else if((old -> s_cause & LINESIX) == LINESIX)
    {
        interruptNum = PRNTINT;
    }
    /* terminal device */
    else if((old -> s_cause & LINESEVEN) == LINESEVEN)
    {
        interruptNum = TERMINT;
    }
    /* invalid interrupt line */
    else 
    {
        PANIC();
    }
    /* get specific device number */
    deviceNum = getDeviceNumber(interruptNum);

    /* get semaphore index */
    i = DEVPERINT * (interruptNum - DEVNOSEM) + deviceNum; 
    
    /* get register for device */
    devRegNum = (device_t *) (INTDEVREG + ((interruptNum-DEVNOSEM)
					* DEVREGSIZE * DEVPERINT) + (deviceNum * DEVREGSIZE));

    /* interrupt was not a terminal */
    if (interruptNum != TERMINT)
    {
        /* store status */
        status = devRegNum -> d_status;
        /* acknowledgee interrupt */
        devRegNum -> d_command = ACK;
    }
    else /* it is a terminal */ 
    {
        /* determine read/write */
        tranStatus = (devRegNum -> t_transm_status & 0xF);
        if(tranStatus != READY)
        {
                status = devRegNum -> t_transm_status;
                devRegNum -> t_transm_command = ACK;
        }
        else
        {
                /* go to next terminal device to handle write */
                i = i + DEVPERINT;
                status = devRegNum -> t_recv_status;
                devRegNum -> t_recv_command = ACK;
        }
    }

    semAdd = &(sem[i]);
    /* increment semaphore */
    ++(*semAdd);
    /* perform the V operation */
    if((*semAdd) <= 0)
    {
        temp = removeBlocked(semAdd);
        
        if(temp != NULL)
        {
            temp -> pcb_semAdd = NULL;
            /* return status from earlier */
            temp -> pcb_state.s_v0 = status;
            softBlockCount--;
            /* insert to ready queue */
            insertProcQ(&(readyQueue), temp);
        }
    }
    /* finish the interrupt */
    finish();
}

/******************************************************************
 * finish
 * 
 * This method is called to exit the interrupt and determine where 
 * to go next. if currentProcess is NULL, call the scheduler for a 
 * new job, if it is not, bill time and reinsert to the ready queue
 * before calling scheduler for a new job.
 *****************************************************************/
HIDDEN void finish()
{
    cpu_t endTime;
    state_PTR oldArea = (state_PTR) INTPOLDAREA;
    /* check for null process */
    if(currentProcess != NULL)
    {
        /* grab time */
        STCK(endTime);
        /* bill the time */
        TODStarted = TODStarted + (endTime - TODStopped);
        /* update the state */
        copyState(oldArea, &(currentProcess -> pcb_state));
        /* insert to ready queue */
        insertProcQ(&readyQueue, currentProcess);
    }
    /* call scheduler for new job */
    scheduler();
}

/******************************************************************
 * getDeviceNumber
 * param: int lineNumber
 * 
 * This method will retreive the highest priority interrupting 
 * device in the bitmap designated by the provided line number.
 * this device is used to get the specific semaphore address of
 * the interrupting device.
 ******************************************************************/
HIDDEN int getDeviceNumber(int lineNumber)
{
    unsigned int bitMap;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    int deviceNum = 0;

    /* adjust linenumber to account for devices with no semaphores */
    lineNumber = lineNumber - DEVNOSEM;
     
     /* grab the line's bitmap */
    bitMap = devReg -> interrupt_dev[lineNumber];

    unsigned int checkingDevice = DEVICEZERO;

    /* array of device addresses to iterate through to find device */
    unsigned int deviceArray[] =
        {
            DEVICEZERO,
            DEVICEONE,
            DEVICETWO,
            DEVICETHREE,
            DEVICEFOUR,
            DEVICEFIVE,
            DEVICESIX,
            DEVICESEVEN
        };

    /* for each device in array, compare bitmap to that address */
    for (deviceNum = 0; deviceNum < DEVPERINT; ++deviceNum)
    {
        /* if searched device is interrupting */
        if ((bitMap & checkingDevice) == checkingDevice)
        {
            /* break to preserve device number */
            break;
        } else {
            checkingDevice << 1;
        }
    }

    /* return device number */
    return deviceNum;
}


