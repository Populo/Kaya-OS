#include "../h/const.h"
#include "../h/types.h"

#include "../e/vmIOsupport.e"
#include "../e/adl.e"
#include "../e/avsl.e"
#include "../e/initProc.e"

#include "/usr/local/include/umps2/umps/libumps.e"

HIDDEN int spinTheBottle();

/* syscalls */
/* ?? */
extern void readWriteBacking(int cylinder, int sector, int head, int readWriteComm, memaddr address);
/* sys 2 but virtual */
extern void meIRL(int procID);
/* write to terminal */
extern void writeTerminal(char* virtAddr, int len, int procID);
/* read from terminal */
extern void readTerminal(char* addr, int procID);
/* print stuff */
extern void writePrinter(char* virtAddr, int len, int procID);
/* read/write to disk */
extern void diskIO(int* blockAddr, int diskNo, int sectNo, int readWrite, int procID);

void vmPrgmHandler() {
    int asid = getCurrentASID();

    /* end it all */
    meIRL(asid);
}


void vmSysHandler()
{
    int callNumber;
    int theGivingID
    state_PTR old;
    cpu_t current;
    cpu_t delay;
    int ID = getCurrentASID();
    int *semAdd;

    old = (state_t *) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];

    callNumber = old -> s_a0;

    switch(callNumber)
    {
        case READTERMINAL:
            readTerminal(old -> s_a1, ID);
            break;
        case WRITETERMINAL:
            writeTerminal(old -> s_a1, old -> s_a2; ID);
            break;
        case VSEMVIRT:
            semAdd = (int *) old -> s_a1;
            (*semAdd)++;

            if(*semAdd <= 0)
            {
                theGivingID = vRemoveBlocked(semAdd);

                if(theGivingID == FAILURE)
                {
                    meIRL(ID);
                }

                SYSCALL(VERHOGEN, &(uProcs[ID-1].Tp_sem), 0, 0);
            }
            break;
        case PSEMVIRT:
            semAdd = (int *) old -> s_a1;

            (*semAdd)--;

            if(*semAdd < 0)
            {
                vInsertBlocked(semAdd, ID);
                SYSCALL(PASSEREN, (int) &(uProcs[ID-1].Tp_sem), 0, 0);
            }
            break;
        case DELAY:
            delay = old -> s_a1;
            
            if(delay < 0)
            {
                meIRL(ID); /* can't delay negative time */
            }
            else if(delay == 0)
            {
                break;    /*Need to ask, but this seems like it would be a waste of fucking time */
            }

            vInsertBlocked(&(uProcs[ID-1].Tp_sem), ID);

            delay = STCK(current) + delay;
            insertDelay(delay, ID);

            SYSCALL(PASSEREN, &(uProcs[ID-1].Tp_sem), 0, 0);
            break;
        case DISK_PUT:
            diskIO(old -> s_a1, old -> s_a2, old -> s_a3, DISK_WRITEBLK, ID);
            break;
        case DISK_GET:
            diskIO(old -> s_a1, old -> s_a2, old -> s_a3, READBLK, ID);
            break;
        case WRITEPRINTER:
            writePrinter(old -> s_a1, old -> s_a2, ID);
            break;
        case GET_TOD:
            STCK(current);
            old -> s_v0 = current;
            break;
        case TERMINATE:
            meIRL(ID);
            break;
    }
    LDST(old);
}


void meIRL(int ID)
{
    int index;
    int kill = FALSE;

    SYSCALL(PASSEREN, (int)&swapSem, 0, 0);

    Interrupts(FALSE);
    for(index = 0; i < SWAPSIZE; index++)
    {
        if(swapPool[index].sw_asid == ID)
        {
            swapPool[index].sw_pte -> pte_entryLO = (swapPool[index].sw_pte->pte_entryLO | ~VALID);
            swapPool[index].sw_asid = -1;
            kill = true;
        }
    }
    if(kill)
    {
        TLBCLR();
    }
    Interrupts(TRUE);
    SYSCALL(VERHOGEN, (int)&swap, 0, 0);
    SYSCALL(VERHOGEN, (int)&sem, 0, 0);

    SYSCALL(TERMINATE_PROCESS, 0, 0, 0);
}


void diskIO(int* blockAddr, int diskNo, int sectNo, int readWrite, int ID)
{
    int head;
    int sec;
    int cyl;
    unsigned int status;
    int* diskbuff;
    state_PTR old;
    devregarea_t* devReg;
    device_t* disk;

    diskbff = (int *)(OSTOP + (diskNo * PAGESIZE));
    old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];
    devReg = (devregarea_t *) RAMBASEADDR;
    disk = &(devReg -> devreg[diskNo]);


    if(diskNo <= 0 || (memaddr)blockAddr < KUSEG2ADDR)
    {
        meIRL(ID);
    }

    head = sectNo % 2;
    sectNo = (sectNo / 2);
    sec = sectNo % 2;
    sectNo = (sectNo / 8);
    cyl = sectNo;

    if(readWrite != DISK_WRITEBLK || readWrite != READBLK)
    {
        PANIC();
    }

    SYSCALL(PASSEREN , (int)&mutexArray[diskNo], 0 , 0);

    if(readWrite == DISK_WRITEBLK)
    {
        copy(blockAddr, diskbuff);
    }

    Interrupts(FALSE);
    disk -> d_command = (cyl << SEEKSHIFT) | DISKSEEK;
    status = SYSCALL(WAITFORIO, DISKINT, diskNo, 0);
    Interrupts(TRUE);    

    if(status == READY)
    {
        Interrupts(FALSE);
        disk -> d_data0 = (memaddr) diskbuff;
        disk -> d_command = (head << HEADSHIFT) | ((sector) << SECTORSHIFT) | readWrite;
        status = SYSCALL(WAITFORIO, DISKINT, diskNo, 0);
        Interrupts(TRUE);
    }
    if(readWrite == READBLK)
    {
        copy(diskbuff, blockaddr);
    }
    
    old -> s_v0 = status;

    SYSCALL(VERHOGEN, (int)&mutexArray[diskNo], 0, 0);
}


void writePrinter(char* virtAddr, int len, int ID)
{
    int i = 0;
    int devNum;
    unsigned int status;
    devregarea_t* devReg;
    device_PTR printer;

    devNum = PRINT0DEV + (ID - 1);
    devReg = (devregarea_t *) RAMBASEADDR;
    printer = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[devNum], 0, 0);

    while(i < len)
    {
        Interrupts(FALSE);
        printer -> d_data0 = (unsinged int) *virtAddr;
        printer -> d_command = PRINTCHAR;
        status = SYSCALL(WAITFORIO, PRNTINT, (ID - 1), 0);
        Interrupts(TRUE);

        if((status & 0xFF) != READY)
        {
            PANIC();
        }

        virtAddr++;
        i++;
    }

    SYSCALL(VERHOGEN, (int)&mutexArray[devNum], 0, 0);
}


void readTerminal(char* addr, int ID)
{
    unsigned int status;
    int i = 0;
    int devNum;
    int bootyCall = FALSE;
    state_PTR old;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    device_t* terminal;

    old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];
    terminal = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[TERMREADSEM + (ID -1)], READTERM);

    while(!bootyCall)
    {
        Interrupts(FALSE);
        terminal -> t_recv_command = RECVCHAR;
        status = SYSCALL(WAITFORIO, TERMINT, (ID -1), READTERM);
        Interrupts(TRUE);

        if(((status & 0XFF00) >> 8)) == (0x0A))
        {
            bootyCall = TRUE;
        }
        else
        {
            *addr = ((status & 0xFF00) >> 8);
            i++;
        }

        if((status & 0xFF) != RECIEVECHAR)
        {
            PANIC();
        }

        addr++
    }

    old -> s_v0 = count;

    SYSCALL(VERHOGEN, (int)&mutexArray[TERMREADSEM + (ID -1)], 0, 0);

}

void writeTerminal(char* virtAddr, int len, int ID)
{
    unsigned int status;
    int i = 0;
    int devNum;
    int bootyCall = FALSE;
    state_PTR old;
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    device_t* terminal;

    old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];
    terminal = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[TERMREADSEM + (ID -1)], WRITETERM);

    while(!bootyCall)
    {
        Interrupts(FALSE);
        terminal -> t_transm_command = TRANSCHAR;
        status = SYSCALL(WAITFORIO, TERMINT, (ID -1), WRITETERM);
        Interrupts(TRUE);

        if(((status & 0XFF00) >> 8)) == (0x0A))
        {
            bootyCall = TRUE;
        }
        else
        {
            *addr = ((status & 0xFF00) >> 8);
            i++;
        }

        if((status & 0xFF) != RECIEVECHAR)
        {
            PANIC();
        }

        addr++
    }

    old -> s_v0 = count;

    SYSCALL(VERHOGEN, (int)&mutexArray[TERMREADSEM + (ID -1)], 0, 0);
}
