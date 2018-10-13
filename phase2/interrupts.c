#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int sem[TOTALSEM];

extern cpu_t TODStarted;

extern void copyState(state_PTR old, state_PTR new);

HIDDEN void finish(cpu_t start);
HIDDEN int getDeviceNumber(unsigned int* bitMap);

void debugL(int i)
{
    int temp;
    temp = i;
}


void ioTrapHandler()
{
    debugL(8999);
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

    oldCause = (oldCause & INTPOLDAREAIDK) >> 8;

    interruptNum = 0;
    debugL(9000);
    if((oldCause & FIRST) != 0)
    {
        debugL(9001);
        debugL(9002);
        PANIC();
    }
    else if((oldCause & SECOND) != 0)
    {
        debugL(9003);
        finish(start);
    }
    else if((oldCause & THIRD) != 0)
    {
        debugL(9004);
        LDIT(INTTIME);
        semAdd = (int*) &(sem[TOTALSEM-1]);
        while(headBlocked(semAdd) != NULL)
        {
            debugL(9005);
            temp = removeBlocked(semAdd);
            debugL(9006);
            STCK(end);
            debugL(9007);
            if(temp != NULL)
            {
                debugL(9008);
                insertProcQ(&readyQueue, temp);
                debugL(9009);
                temp -> cpu_time = temp -> cpu_time + (end + start);
                softBlockCount--;
            }
        }
        (*semAdd) = 0;
        finish(start);
        debugL(9010);
    }  
    else if((oldCause & FOURTH) != 0)
    {
        debugL(9011);
        interruptNum = DISKINT;
    }
    else if((oldCause & FIFTH) != 0)
    {
        debugL(9012);
        interruptNum = TAPEINT;
    }
    else if((oldCause & SIXTH) != 0)
    {
        debugL(9013);
        interruptNum = NETWINT;
    }
    else if((oldCause & SEVENTH) != 0)
    {
        debugL(9014);
        interruptNum = PRNTINT;
    }
    else if((oldCause & EIGHTH) != 0)
    {
        debugL(9015);
        interruptNum = TERMINT;
    }
    else 
    {
        debugL(9016);
        PANIC();
    }
    debugL(9017);
    deviceNum = getDeviceNumber((unsigned int*) (INTBITMAP + ((interruptNum - DEVNOSEM) * WORDLEN)));
    debugL(9018);
    if(deviceNum == -1)
    {
        debugL(9019);
        PANIC();
    }
    debugL(9020);
    devRegNum = (device_t *) (INTDEVREG + ((interruptNum - DEVNOSEM) * DEVREGSIZE * DEVPERINT) + (deviceNum + DEVREGSIZE));
    debugL(9021);
    if(interruptNum != 7)
    {
        debugL(9022);
        status = devRegNum -> d_status;
        devRegNum -> d_command = ACK;
        i = DEVPERINT * (interruptNum - DEVNOSEM) + deviceNum;
    }
    else
    {
        debugL(9023);
        tranStatus = (devRegNum -> t_transm_status & 0xFF);

        if(tranStatus == 3 || tranStatus == 4 || tranStatus == 5)
        {
            debugL(9024);
            i = (DEVPERINT * (interruptNum - DEVNOSEM)) + deviceNum;
            status = devRegNum -> t_recv_status;
            devRegNum -> t_recv_command = ACK;
        } 
        else
        {
            debugL(9025);
            i = DEVPERINT * (interruptNum - DEVNOSEM + 1) + deviceNum;
            status = devRegNum -> t_recv_status;
            devRegNum -> t_recv_command = ACK;
        }
    }
    debugL(9026);
    semAdd = &(sem[i]);
    ++(*semAdd);

    if((*semAdd) <= 0)
    {
        debugL(9027);
        temp = removeBlocked(semAdd);
        if(temp != NULL)
        {
            debugL(9028);
            temp -> pcb_s.s_v0 = status;
            insertProcQ(&readyQueue, temp);
            softBlockCount--;
        }
        
    }
    else
    {
        /* LOL FUCK YOU */
    }
    debugL(9029);
    finish(start);
}

HIDDEN void finish(cpu_t start)
{
    debugL(9030);
        cpu_t end;
        state_PTR old = (state_PTR) INTPOLDAREA;
        if(currentProcess != NULL)
        {
            debugL(9031);
            STCK(end);
            TODStarted = TODStarted + (end - start);
            copyState(old, &(currentProcess -> pcb_s));
            insertProcQ(&readyQueue, currentProcess);
        }
        debugL(9032);
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

