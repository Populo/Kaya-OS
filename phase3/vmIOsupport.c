#include "../h/const.h"
#include "../h/types.h"

#include "../e/vmIOsupport.e"
#include "../e/adl.e"
#include "../e/avsl.e"
#include "../e/initProc.e"

#include "/usr/local/include/umps2/umps/libumps.e"

HIDDEN int spinTheBottle();

extern uProc_PTR uProcs[8];
extern int swapSem;
extern int mutexArray[MAXPROC];
extern int masterSem;
extern swap_t swapPool[SWAPSIZE];

/* syscalls */
/* ?? */
HIDDEN void readWriteBacking(int cylinder, int sector, int head, int readWriteComm, memaddr address);
/* sys 2 but virtual */
HIDDEN void meIRL(int procID);
/* write to terminal */
HIDDEN void writeTerminal(char* virtAddr, int len, int procID);
/* read from terminal */
HIDDEN void readTerminal(char* addr, int procID);
/* print stuff */
HIDDEN void writePrinter(char* virtAddr, int len, int procID);
/* read/write to disk */
HIDDEN void diskIO(int* blockAddr, int diskNo, int sectNo, int readWrite, int procID);

extern putALoadInMeDaddy(state_PTR state);

void vmPrgmHandler() {
    int asid = getCurrentASID();

    /* end it all */
    meIRL(asid);
}

void vmMemHandler() {
    int missingSegment,
        missingPage,
        newFrame,
        currentPage,
        currentASID,
        missingASID;
    
    devregarea_t *device = (devregarea_t *)RAMBASEADDR;
    memaddr RAMTOP = device -> rambase + device -> ramsize;
    memaddr SWAPPOOL = RAMTOP - (2 * PAGESIZE) - (SWAPSIZE * PAGESIZE);

    missingASID = getCurrentASID();

    state_PTR oldState = (state_PTR) &(uProcs[missingASID-1] -> uProc_states[TLBTRAP][OLD]);

    int cause = (oldState -> s_cause & INTCAUSEMASK) >> 2;

    if ((cause != TLBL) && (cause != TLBS)) {
        meIRL(missingASID);
    }

    missingSegment = oldState -> s_entryHI >> SHIFT_SEG;
    missingPage = oldState -> s_entryHI >> SHIFT_PFN;

    if (missingPage >= KUSEGSIZE) {
        missingPage = KUSEGSIZE - 1;
    }

    SYSCALL(PASSEREN,
            (int)&swapSem,
            0,0);

    newFrame = spinTheBottle();

    memaddr swapAddress = SWAPPOOL + (newFrame * PAGESIZE);

    swap_t *swapFrame = &swapPool[newFrame];

    if (swapFrame -> sw_asid != -1) {
        Interrupts(FALSE);

        swapFrame -> sw_pte -> entryLO = swapFrame -> sw_pte -> entryLO & nVALID;

        TLBCLR();
        Interrupts(TRUE);

        currentASID = swapFrame -> sw_asid;
        currentPage = swapFrame -> sw_pgNum;

        readWriteBacking(currentPage, currentASID, DISK0, DISK_WRITEBLK, swapAddress);
    }

    readWriteBacking(missingPage, missingASID, DISK0, DISK_READBLK, swapAddress);

    Interrupts(FALSE);

    swapFrame -> sw_asid = missingASID;
    swapFrame -> sw_segNum = missingSegment;
    swapFrame -> sw_pgNum = missingPage;

    if (missingSegment == SEG3) {
        swapFrame -> sw_pte = &(kuSeg3.pteTable[missingPage]);
        swapFrame -> sw_pte -> entryLO = swapAddress | VALID | DIRTY | GLOBAL;
    } else {
        swapFrame -> sw_pte = &(uProcs[missingASID - 1].uProc_pte.pteTable[missingPage]);
        swapFrame -> sw_pte -> entryLO = swapAddress | VALID | DIRTY;
    }

    TLBCLR();
    Interrupts(TRUE);

    SYSCALL(VERHOGEN,
            (int)&swapSem,
            0, 0);

    putALoadInMeDaddy(oldState);
}

void vmSysHandler()
{
    int callNumber;
    int theGivingID;
    state_PTR old;
    cpu_t current;
    cpu_t delay;
    int ID = getCurrentASID();
    int *semAdd;

    old = (state_PTR) &(uProcs[ID-1] -> uProc_states[SYSTRAP][OLD]);

    callNumber = old -> s_a0;

    switch(callNumber)
    {
        case READTERMINAL:
            readTerminal((char *) old -> s_a1, ID);
            break;
        case WRITETERMINAL:
            writeTerminal((char *) old -> s_a1, old -> s_a2, ID);
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

                SYSCALL(VERHOGEN, &(uProcs[ID-1] -> uProc_semAdd), 0, 0);
            }
            break;
        case PSEMVIRT:
            semAdd = (int *) old -> s_a1;

            (*semAdd)--;

            if(*semAdd < 0)
            {
                vInsertBlocked(semAdd, ID);
                SYSCALL(PASSEREN, (int) &(uProcs[ID-1] -> uProc_semAdd), 0, 0);
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

            vInsertBlocked(&(uProcs[ID-1] -> uProc_semAdd), ID);

            delay = STCK(current) + delay;
            insertDelay(delay, ID);

            SYSCALL(PASSEREN, &(uProcs[ID-1] -> uProc_semAdd), 0, 0);
            break;
        case DISK_PUT:
            diskIO((int *) old -> s_a1, old -> s_a2, old -> s_a3, DISK_WRITEBLK, ID);
            break;
        case DISK_GET:
            diskIO((int *) old -> s_a1, old -> s_a2, old -> s_a3, DISK_READBLK, ID);
            break;
        case WRITEPRINTER:
            writePrinter((char *) old -> s_a1, old -> s_a2, ID);
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
    for(index = 0; index < SWAPSIZE; index++)
    {
        if(swapPool[index].sw_asid == ID)
        {
            swapPool[index].sw_pte -> entryLO = (swapPool[index].sw_pte -> entryLO | ~VALID);
            swapPool[index].sw_asid = -1;
            kill = TRUE;
        }
    }
    if(kill)
    {
        TLBCLR();
    }
    Interrupts(TRUE);
    SYSCALL(VERHOGEN, (int)&swapSem, 0, 0);
    SYSCALL(VERHOGEN, (int)&masterSem, 0, 0);

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

    diskbuff = (int *)(OSCODEEND + (diskNo * PAGESIZE));
    old = (state_PTR) &(uProcs[ID-1] -> uProc_states[SYSTRAP][OLD]);
    devReg = (devregarea_t *) RAMBASEADDR;
    disk = &(devReg -> devreg[diskNo]);


    if(diskNo <= 0 || (memaddr)blockAddr < SEG2)
    {
        meIRL(ID);
    }

    head = sectNo % 2;
    sectNo = (sectNo / 2);
    sec = sectNo % 2;
    sectNo = (sectNo / 8);
    cyl = sectNo;

    if(readWrite != DISK_WRITEBLK || readWrite != DISK_READBLK)
    {
        PANIC();
    }

    SYSCALL(PASSEREN , (int)&mutexArray[diskNo], 0 , 0);

    if(readWrite == DISK_WRITEBLK)
    {
        copy(blockAddr, diskbuff);
    }

    Interrupts(FALSE);
    disk -> d_command = (cyl << SHIFT_SEEK) | DISK_SEEKCYL;
    status = SYSCALL(WAITIO, DISKINT, diskNo, 0);
    Interrupts(TRUE);    

    if(status == READY)
    {
        Interrupts(FALSE);
        disk -> d_data0 = (memaddr) diskbuff;
        disk -> d_command = (head << SHIFT_HEAD) | ((sec) << SHIFT_SECTOR) | readWrite;
        status = SYSCALL(WAITIO, DISKINT, diskNo, 0);
        Interrupts(TRUE);
    }
    if(readWrite == DISK_READBLK)
    {
        copy(diskbuff, blockAddr);
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
    device_t* printer;

    devNum = PRINTDEV + (ID - 1);
    devReg = (devregarea_t *) RAMBASEADDR;
    printer = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[devNum], 0, 0);

    while(i < len)
    {
        Interrupts(FALSE);
        printer -> d_data0 = (unsigned int) *virtAddr;
        printer -> d_command = PRINTCHAR;
        status = SYSCALL(WAITIO, PRNTINT, (ID - 1), 0);
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

    old = (state_PTR) &uProcs[ID-1] -> uProc_states[SYSTRAP][OLD];
    terminal = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[TERMREADSEM + (ID -1)], READTERM, 0);

    while(!bootyCall)
    {
        Interrupts(FALSE);
        terminal -> t_recv_command = RECVCHAR;
        status = SYSCALL(WAITIO, TERMINT, (ID -1), READTERM);
        Interrupts(TRUE);

        if(((status & 0XFF00) >> 8) == (0x0A))
        {
            bootyCall = TRUE;
        }
        else
        {
            *addr = ((status & 0xFF00) >> 8);
            i++;
        }

        if((status & 0xFF) != RECVCHAR)
        {
            PANIC();
        }

        addr++;
    }

    old -> s_v0 = i;

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

    old = (state_PTR) &uProcs[ID-1] -> uProc_states[SYSTRAP][OLD];
    terminal = &(devReg -> devreg[devNum]);

    SYSCALL(PASSEREN, (int)&mutexArray[KUSEGSIZE + (ID -1)], WRITETERM, 0);

    while(!bootyCall)
    {
        Interrupts(FALSE);
        terminal -> t_transm_command = TRANSCHAR;
        status = SYSCALL(WAITIO, TERMINT, (ID -1), WRITETERM);
        Interrupts(TRUE);

        if(((status & 0XFF00) >> 8) == (0x0A))
        {
            bootyCall = TRUE;
        }
        else
        {
            *virtAddr = ((status & 0xFF00) >> 8);
            i++;
        }

        if((status & 0xFF) != RECVCHAR)
        {
            PANIC();
        }

        *virtAddr++;
    }

    old -> s_v0 = i;

    SYSCALL(VERHOGEN, (int)&mutexArray[TERMREADSEM + (ID -1)], 0, 0);
}

HIDDEN int spinTheBottle() 
{
    cpu_t seed;

    STCK(seed);

    return (int) seed % SWAPSIZE;
}

