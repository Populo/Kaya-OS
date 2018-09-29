#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"

#include "../e/interrupts.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currProc;
extern pcb_PTR readyQueue;
extern int sem[TOTALSEM];

void ioTrapHandler()
{
    unsigned int oldCause;
    cpu_t start, end;
    int deviceNum, interruptNum;
    device_t *devRegNum;
    int i, status, tranStatus;
    int* semAdd;
    pcb_PTR temp;
    state_PTR old;

    old = (state_PTR) INTPOLDAREA;

    STCK(start);

    oldCause = old -> s_cause;

    oldCause = (oldCause & INTPOLDAREA) >> 8;

    interruptNum = 0;

    if((oldCause & FIRST) != 0)
    {
        PANIC();
    }
    else if((oldCause & SECOND) != 0)
    {
        finish(start);
    }
    else if((oldCause & THIRD) != 0)
    {
        ldit(INTTIME);
        semAdd = (int*) &(sem[TOTALSEM-1]);
        while(headBlocked(semAdd) != NULL)
        {
            temp = removeBlocked(semAdd);
            STCK(end);
            if(temp != NULL)
            {
                insertProcQ(&readyQueue, temp);

                temp -> cpu_time = temp -> cpu_time + (end + start);
                softBlockCount--;
            }
        }
        (*semAdd) = 0;
        finish(start);
    }
    else if((oldCause & FOURTH) != 0)
    {
        interruptNum = DISKINT;
    }
    else if((oldCause & FIFTH) != 0)
    {
        interruptNum = TAPEINT;
    }
    else if((oldCause & SIXTH) != 0)
    {
        interruptNum = NETWINT;
    }
    else if((oldCause & SEVENTH) != 0)
    {
        interruptNum = PRNTINT;
    }
    else if((oldCause & EIGHTH) != 0)
    {
        interruptNum = TERMINT;
    }
    else
    {
        PANIC();
    }

    deviceNum = getDeviceNum((unsigned int*) (INTBITMAP + ((interruptNum - DEVNOSEM) * WORDLEN)));

    if(deviceNum == -1)
    {
        PANIC();
    }

    devRegNum = (device_t *) (INTDEVREG + ((interruptNum - DEVNOSEM) * DEVREGSIZE * DEVPERINT) + (deviceNum + DEVREGSIZE));

    if(interruptNum != 7)
    {
        status = devRegNum -> d_status;
        devRegNum -> d_command = ACK;
        i = DEVPERINT * (interruptNum - DEVNOSEM) + deviceNum;
    }
    else
    {
        tranStatus = (devRegNum -> /* bitch i dont know */ )
    }
}
