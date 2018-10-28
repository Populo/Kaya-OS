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

void debugL(int i)
{
    int temp;
    temp = i;
}

void debugk(int i)
{
    int temp;
    temp = i;
}

void debugREEE(int u)
{
    int fuck;
    fuck = u;
}

void debugDevice(int line, int device, int index, int sem)
{
    int ree;
    ree = index;
}

void ioTrapHandler()
{
    STCK(fuckyourClock);
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

    /* worry about this break later */
    i = DEVPERINT * (interruptNum - DEVNOSEM) + deviceNum; 
    
    debugk(1000);
    devRegNum = (device_t *) (INTDEVREG + ((interruptNum-DEVNOSEM)
					* DEVREGSIZE * DEVPERINT) + (deviceNum * DEVREGSIZE));

    debugk(1001);
    if (interruptNum != TERMINT)
    {
        status = devRegNum -> d_status;

        devRegNum -> d_command = ACK;
    }
    else 
    {
        tranStatus = (devRegNum -> t_transm_status & 0xFF);
        /* debugk(tranStatus); */
        switch (tranStatus)
        {
            case 3:
            case 4:
            case 5:
                debugNickStone(55);
                status = devRegNum -> t_transm_status;
                devRegNum -> t_transm_command = ACK;
                break;
            default:
                debugNickStone(66);
                i = i + DEVPERINT;
                status = devRegNum -> t_recv_status;
                devRegNum -> t_recv_command = ACK;
                break;
        }
    }

    /* Not a terminal */
    

    semAdd = &(sem[i]);
    ++(*semAdd);
    
    debugDevice(interruptNum, deviceNum, i, *semAdd);
    debugL(20);
    debugREEE(sem[i]);
    if((*semAdd) <= 0)
    {
        debugL(21);
        temp = removeBlocked(semAdd);
        
        if(temp != NULL)
        {
            temp -> pcb_semAdd = NULL;
            debugL(22);
            temp -> pcb_state.s_v0 = status;
            softBlockCount--;
            debugL(23);
            insertProcQ(&(readyQueue), temp);
            debugL(24);
        }
        debugL(25);
    }

    debugL(9029);
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
        debugL(9031);
        copyState(oldArea, &(currentProcess -> pcb_state));
        LDST(&(currentProcess -> pcb_state));
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
        if (bitMap == deviceArray[deviceNum])
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


