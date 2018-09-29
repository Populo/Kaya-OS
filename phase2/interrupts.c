#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currProc;
extern pcb_PTR readyQueue;
extern int sem[TOTALSEM];

extern cpu_t TODStarted;

extern void copyState(state_PTR old, state_PTR new);

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
        LDIT(INTTIME);
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
        tranStatus = (devRegNum -> t_transm_status & 0xFF);

        if(tranStatus == 3 || tranStatus == 4 || tranStatus == 5)
        {
            i = (DEVPERINT * (interruptNum - DEVNOSEM)) + deviceNum;
            status = devRegNum -> t_recv_status;
            devRegNum -> t_recv_command = ACK;
        } 
        else
        {
            i = DEVPERINT * (interruptNum - DEVNOSEM + 1) + deviceNum;
            status = devRegNum -> t_recv_status;
            devRegNum -> t_recv_command = ACK;
        }
    }
    semAdd = &(sem[i]);
    ++(*semAdd);

    if((*semAdd) <= 0)
    {
        temp = removeBlocked(semAdd);
        if(temp != NULL)
        {
            temp -> pcb_s.s_v0 = status;
            insertProcQ(&readyQueue, temp);
            softBlockCount--;
        }
        
    }
    else
    {
        /* LOL FUCK YOU */
    }

    finish(start);
}

HIDDEN void finish(cpu_t start)
{
        cpu_t end;
        state_PTR old = (state_PTR) INTPOLDAREA;
        if(currentProcess != NULL)
        {
                STCK(end);
                TODStarted = TODStarted + (end - start);
                copyState(old, &(currentProcess -> pcb_s));
                insertProcQ(&readyQueue, currentProcess);
        }
        scheduler();
}

HIDDEN int getDeviceNumber(unsigned int* bitMap) {
	unsigned int oldCause = *bitMap;
	if((oldCause & FIRST) != 0) {
		return 0;
	}

	else if((oldCause & SECOND) != 0){
		return 1;
	}

	else if((oldCause & THIRD) != 0){
		return 2;
	}

	else if((oldCause & FOURTH) != 0){
		return 3;
	}

	else if((oldCause & FIFTH) != 0){
		return 4;
	}

	else if((oldCause & SIXTH) != 0){
		return 5;
	}

	else if((oldCause & SEVENTH) != 0){
		return 6;
	}

	else if((oldCause & EIGHTH) != 0){
		return 7;
	}

	return -1;
}
