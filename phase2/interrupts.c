#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"


extern cpu_t TODStarted;
cpu_t fuckyourClock;

extern void copyState(state_PTR old, state_PTR new);
extern void sysVerhogen(state_PTR old);

HIDDEN void finish();
HIDDEN int getDeviceNumber(int lineNumber);

extern int sem[TOTALSEM];


void ioTrapHandler()
{
    STCK(fuckyourClock);
    unsigned int oldCause;
    cpu_t start, end, total;
    int deviceNum, interruptNum;
    device_t* devRegNum;
    int i, status, tranStatus;
    int* semAdd;
    pcb_PTR temp;
    state_PTR old = (state_PTR) INTPOLDAREA;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;



    if((old -> s_cause & LINEZERO) == LINEZERO)
    {
        PANIC();
    }
    else if((old -> s_cause & LINEONE) == LINEONE)
    {
        if(currentProcess != NULL)
        {
            insertProcQ(&(readyQueue), currentProcess);
            currentProcess = NULL;
        }
        scheduler();
    }
    else if((old -> s_cause & LINETWO) == LINETWO)
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
                temp -> pcb_time = (temp -> pcb_time) + (end - fuckyourClock);
                softBlockCount--;
            }
        }
        (*semAdd) = 0;
        finish();
    }  
    else if((old -> s_cause & LINETHREE) == LINETHREE)
    {
        interruptNum = DISKINT;
    }
    else if((old -> s_cause & LINEFOUR) == LINEFOUR)
    {
        interruptNum = TAPEINT;
    }
    else if((old -> s_cause & LINEFIVE) == LINEFIVE)
    {
        interruptNum = NETWINT;
    }
    else if((old -> s_cause & LINESIX) == LINESIX)
    {
        interruptNum = PRNTINT;
    }
    else if((old -> s_cause & LINESEVEN) == LINESEVEN)
    {
        interruptNum = TERMINT;
    }
    else 
    {
        PANIC();
    }
    deviceNum = getDeviceNumber(interruptNum);

    /* worry about this break later */
    i = DEVPERINT * (interruptNum - DEVNOSEM) + deviceNum; 
    
    devRegNum = (device_t *) (INTDEVREG + ((interruptNum-DEVNOSEM)
					* DEVREGSIZE * DEVPERINT) + (deviceNum * DEVREGSIZE));

    if (interruptNum != TERMINT)
    {
        status = devRegNum -> d_status;

        devRegNum -> d_command = ACK;
    }
    else 
    {
        tranStatus = (devRegNum -> t_transm_status & 0xFF);
        switch (tranStatus)
        {
            case 3:
            case 4:
            case 5:
                status = devRegNum -> t_transm_status;
                devRegNum -> t_transm_command = ACK;
                break;
            default:
                i = i + DEVPERINT;
                status = devRegNum -> t_recv_status;
                devRegNum -> t_recv_command = ACK;
                break;
        }
    }

    /* Not a terminal */
    

    semAdd = &(sem[i]);
    ++(*semAdd);
    if((*semAdd) <= 0)
    {
        temp = removeBlocked(semAdd);
        
        if(temp != NULL)
        {
            temp -> pcb_semAdd = NULL;
            temp -> pcb_state.s_v0 = status;
            softBlockCount--;
            insertProcQ(&(readyQueue), temp);
        }
    }
    finish();
}

HIDDEN void finish()
{
    cpu_t endTime;
    state_PTR oldArea = (state_PTR) INTPOLDAREA;
    if(currentProcess != NULL)
    {
        STCK(endTime);
        TODStarted = TODStarted + (endTime - fuckyourClock);
        copyState(oldArea, &(currentProcess -> pcb_state));
        insertProcQ(&readyQueue, currentProcess);
    }
    scheduler();
}


HIDDEN int getDeviceNumber(int lineNumber)
{
    unsigned int bitMap;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    unsigned int currentDevice = DEVICEONE;
    int deviceNum = 0;
    int found = FALSE;

    lineNumber = lineNumber - DEVNOSEM;
     
    bitMap = devReg -> interrupt_dev[lineNumber];

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

    for (deviceNum = 0; deviceNum < DEVPERINT; ++deviceNum)
    {
        if ((bitMap & deviceArray[deviceNum]) == deviceArray[deviceNum])
        {
            break;
        }
    }
    
    /* while(!found)
    {
        if((currentDevice & bitMap) == currentDevice)
        {
            found = TRUE;
        }
        else
        {
            currentDevice =  currentDevice << 1;
            deviceNum++;
        }

    } */
    return deviceNum;
}


