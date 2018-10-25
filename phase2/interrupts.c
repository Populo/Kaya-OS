#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"


extern cpu_t TODStarted;

extern void copyState(state_PTR old, state_PTR new);
extern void sysVerhogen(state_PTR old);

HIDDEN void finish();
HIDDEN int getDeviceNumber(int lineNumber);

extern int sem[TOTALSEM];

void debugL(int i)
{
    int temp;
    temp = i;
}

void debugREEE(int u)
{
    int fuck;
    fuck = u;
}


void ioTrapHandler()
{
    
    debugL(8999);
    unsigned int oldCause;
    cpu_t start, end, total;
    int deviceNum, interruptNum;
    device_t* devRegNum;
    int i, status, tranStatus;
    int* semAdd;
    pcb_PTR temp;
    state_PTR old = (state_PTR) INTPOLDAREA;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;

if(currentProcess != NULL)
{
    debugREEE(9999);
    STCK(end);
    debugREEE(58008);
    total = end - TODStarted;

    currentProcess -> pcb_time = currentProcess -> pcb_time + total;

    copyState(old, &(currentProcess -> pcb_state));
}



    if((old -> s_cause & LINEZERO) == LINEZERO)
    {
        debugL(9002);
        PANIC();
    }
    else if((old -> s_cause & LINEONE) == LINEONE)
    {
        if(currentProcess != NULL)
        {
            insertProcQ(&(readyQueue), currentProcess);
            currentProcess = NULL;
        }
        setTIMER(QUANTUM);
        scheduler();
    }
    else if((old -> s_cause & LINETWO) == LINETWO)
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
    else if((old -> s_cause & LINETHREE) == LINETHREE)
    {
        debugL(9011);
        interruptNum = DISKINT;
    }
    else if((old -> s_cause & LINEFOUR) == LINEFOUR)
    {
        debugL(9012);
        interruptNum = TAPEINT;
    }
    else if((old -> s_cause & LINEFIVE) == LINEFIVE)
    {
        debugL(9013);
        interruptNum = NETWINT;
    }
    else if((old -> s_cause & LINESIX) == LINESIX)
    {
        debugL(9014);
        interruptNum = PRNTINT;
    }
    else if((old -> s_cause & LINESEVEN) == LINESEVEN)
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
    if(deviceNum == -1)
    {
        debugL(9019);
        PANIC();
    }
    interruptNum = interruptNum - DISKINT;

    i = (DEVPERINT * interruptNum) + deviceNum;

    devRegNum = &(devReg -> devreg[i]);

    sem[i] = sem[i] + 1;
    debugREEE(sem[i]);
    if(sem[i] <= 0)
    {
        temp = removeBlocked(&(sem[i]));
        if(temp != NULL)
        {
            temp -> pcb_semAdd = NULL;

            temp -> pcb_state.s_v0 = devRegNum -> t_transm_status;

            softBlockCount--;

            insertProcQ(&(readyQueue), temp);
        }
/*         else
        {
            sem[i] = devRegNum -> t_transm_status;
        } */
    }

    devRegNum -> t_transm_command = ACK;
    debugL(9029);
    finish();
}

HIDDEN void finish()
{
    if(currentProcess != NULL)
    {
        debugL(9031);
        /*currentProcess = NULL;*/
        LDST((state_PTR) INTPOLDAREA);
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

/*
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
*/

    