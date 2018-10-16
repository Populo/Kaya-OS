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

HIDDEN void finish();
HIDDEN int getDeviceNumber(int lineNumber);

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
    device_t* devRegNum;
    int i, status, tranStatus;
    int* semAdd;
    pcb_PTR temp;
    state_PTR old;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;

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
        if(currentProcess != NULL)
        {
            insertProcQ(&(readyQueue), currentProcess);
            currentProcess = NULL;
        }
        setTIMER(QUANTUM);
        scheduler();
    }
    else if((oldCause & THIRD) != 0)
    {
        debugL(9004);
        semAdd = (int*) &(sem[TOTALSEM-1]);
        while(headBlocked(semAdd) != NULL)
        {
            debugL(9005);
            temp = removeBlocked(semAdd);
            debugL(9007);
            if(temp != NULL)
            {
                debugL(9008);
                insertProcQ(&readyQueue, temp);
                debugL(9009);
                temp -> pcb_time = (temp -> pcb_time) + (end + start);
                softBlockCount--;
            }
        }
        (*semAdd) = 0;
        LDIT(INTTIME);
        finish();
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
    deviceNum = getDeviceNumber(interruptNum);
    if(interruptNum == TERMINT)
    {
        goPowerRangers();
    }
    debugL(9018);
    if(deviceNum == -1)
    {
        debugL(9019);
        PANIC();
    }
    interruptNum = interruptNum - DISKINT;

    i = (DEVPERINT * interruptNum) + deviceNum;

    devRegNum = &(devReg -> devreg[i]);

    sem[i] = sem[i] + 1;

    if(sem[i] <= 0)
    {
        temp = removeBlocked(&(sem[i]));
        if(temp != NULL)
        {
            temp -> pcb_semAdd = NULL;

            temp -> pcb_s.s_v0 = devRegNum -> d_status;
            softBlockCount--;

            insertProcQ(&(readyQueue), temp);
        }
        else
        {
            sem[i] = devRegNum -> d_status;
        }
    }
    dev -> d_command = ACK;

    debugL(9029);
    finish();
}

HIDDEN void finish()
{
    if(currentProcess != NULL)
    {
        STCK(TODStarted);
        LDST(&(currentProcess -> pcb_s));
    }
    debugL(9032);
    scheduler();
}


HIDDEN int getDeviceNumber(int lineNumber)
{
    unsigned int bitMap;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    unsigned int currentDevice = DEVICEONE;
    int deviceNum = 0;
    int found = FALSE;

    lineNumber = lineNumber -3;
     
    bitMap = devReg -> interrupt_dev[lineNumber];

    while(!found)
    {
        if((currentDevice & bitMap) == currentDevice)
        {
            found = TRUE;
        }
        else
        {
            currentDevice = currentDevice << 1;
            deviceNum++;
        }

    }
    return deviceNum;
}

void goPowerRangers(int deviceNum)
{
    pcb_PTR process;
    int semAdd = (TERMINT - DISKINT)* DEVPERINT + deviceNum;
    int receive = TRUE;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    device_t* dev = &(devReg -> devreg[semAdd]);

    if((dev -> t_transm_status & 0x0F) != READY)
    {
        semAdd = semAdd + DEVPERINT;
        receive = FALSE;
    }

    sem[semAdd] = sem[semAdd] + 1;

    if(sem[semAdd] <= 0)
    {
        process = removeBlocked(&(sem[semAdd]));
        if(process != NULL)
        {
            process -> pcb_semAdd = NULL;

            if(receive)
            {
                process -> pcb_s.s_v0 = dev->t_recv_status;
                dev->t_recv_command = ACK;
            }

            else
            {
                process -> pcb_semAdd = NULL;

                process -> pcb_s.s_v0 = dev->t_transm_status;
                dev-> t_transm_command = ACK;
            }

            softBlockCount--;

            insertProcQ(&(readyQueue), process);
        }
    }

    finish();
}


    