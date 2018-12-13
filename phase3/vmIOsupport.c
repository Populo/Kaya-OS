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


extern void putALoadInMeDaddy(state_PTR state);

void debugVM(int a) {
    int f;
    f = a;
}

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

    state_PTR oldState = (state_PTR) &(uProcs[missingASID-1].uProc_states[TLBTRAP][OLD]);

    int cause = (oldState->s_cause & INTCAUSEMASK) >> 2;

    if ((cause != TLBL) && (cause != TLBS)) {
        meIRL(missingASID);
    }

    missingSegment = (oldState->s_asid >> SHIFT_SEG);
    missingPage = ((oldState->s_asid & GET_VPN) >> SHIFT_VPN);

    if (missingPage >= KUSEGSIZE) {
        missingPage = KUSEGSIZE - 1;
    }

    debugVM(100);

    SYSCALL(PASSEREN, (int)&swap, 0, 0);

    newFrame = spinTheBottle();

    memaddr swapAddress = SWAPPOOL - (newFrame * PAGESIZE);

    debugVM(150);

    if (swapPool[newFrame].sw_asid != -1) {
        Interrupts(FALSE);

        swapPool[newFrame].sw_pte -> entryLO = swapPool[newFrame].sw_pte -> entryLO & nVALID;

        TLBCLR();
        Interrupts(TRUE);

        currentASID = swapPool[newFrame].sw_asid;
        currentPage = swapPool[newFrame].sw_pgNum;
        debugVM(1);
        readWriteBacking(currentASID, currentPage, DISK0, DISK_WRITEBLK, swapAddress);
    }

    debugVM(200);

    readWriteBacking(missingASID, missingPage, DISK0, DISK_READBLK, swapAddress);

    debugVM(250);

    Interrupts(FALSE);

    swapPool[newFrame].sw_asid = missingASID;
    swapPool[newFrame].sw_segNum = missingSegment;
    swapPool[newFrame].sw_pgNum = missingPage;

    if (missingSegment == SEG3) {
        swapPool[newFrame].sw_pte = &(kuSeg3.pteTable[missingPage]);
        swapPool[newFrame].sw_pte -> entryLO = swapAddress | VALID | DIRTY | GLOBAL;
    } else {
        swapPool[newFrame].sw_pte = &(uProcs[missingASID - 1].uProc_pte.pteTable[missingPage]);
        swapPool[newFrame].sw_pte -> entryLO = swapAddress | VALID | DIRTY;
    }

    debugVM(300);

    TLBCLR();
    Interrupts(TRUE);

    SYSCALL(VERHOGEN, (int)&swap, 0, 0);
    debugA(12);
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

    old = (state_PTR) &(uProcs[ID-1].uProc_states[SYSTRAP][OLD]);

    callNumber = old->s_a0;

    switch(callNumber)
    {
        case READTERMINAL:
            readTerminal((char *) old->s_a1, ID);
            break;
        case WRITETERMINAL:
            writeTerminal((char *) old->s_a1, old->s_a2, ID);
            break;
        case VSEMVIRT:
            semAdd = (int *) old->s_a1;
            (*semAdd)++;

            if(*semAdd <= 0)
            {
                theGivingID = vRemoveBlocked(semAdd);

                if(theGivingID == FAILURE)
                {
                    meIRL(ID);
                }

                SYSCALL(VERHOGEN, &(uProcs[theGivingID-1].uProc_semAdd), 0, 0);
            }
            break;
        case PSEMVIRT:
            semAdd = (int *) old->s_a1;

            (*semAdd)--;

            if(*semAdd < 0)
            {
                vInsertBlocked(semAdd, ID);
                SYSCALL(PASSEREN, (int) &(uProcs[ID-1].uProc_semAdd), 0, 0);
            }
            break;
        case DELAY:
            delay = old->s_a1 * 1000000;
            
            if(delay < 0)
            {
                meIRL(ID); /* can't delay negative time */
            }

            vInsertBlocked(&(uProcs[ID-1].uProc_semAdd), ID);

            delay = STCK(current) + delay;
            insertDelay(delay, ID);

            SYSCALL(PASSEREN,(int) &(uProcs[ID-1].uProc_semAdd), 0, 0);
            break;
        case DISK_PUT:
            diskIO((int *) old->s_a1, old->s_a2, old->s_a3, DISK_WRITEBLK, ID);
            break;
        case DISK_GET:
            diskIO((int *) old->s_a1, old->s_a2, old->s_a3, DISK_READBLK, ID);
            break;
        case WRITEPRINTER:
            writePrinter((char *) old->s_a1, old->s_a2, ID);
            break;
        case GET_TOD:
            STCK(current);
            old->s_v0 = current;
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

    SYSCALL(PASSEREN, (int)&swap, 0, 0);

    Interrupts(FALSE);
    for(index = 0; index < SWAPSIZE; index++)
    {
        if(swapPool[index].sw_asid == ID)
        {
            swapPool[index].sw_pte -> entryLO = (swapPool[index].sw_pte -> entryLO | nVALID);
            swapPool[index].sw_asid = -1;
            kill = TRUE;
        }
    }
    if(kill)
    {
        TLBCLR();
    }
    Interrupts(TRUE);
    SYSCALL(VERHOGEN, (int)&swap, 0, 0);
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
    old = (state_PTR) &(uProcs[ID-1].uProc_states[SYSTRAP][OLD]);
    devReg = (devregarea_t *) RAMBASEADDR;
    disk = &(devReg -> devreg[diskNo]);


    if(diskNo <= 0 || (memaddr)blockAddr < SEG2)
    {
        meIRL(ID);
    }

    head = sectNo % 2;
    sectNo = (sectNo / 2);
    sec = sectNo % 8;
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
    
    old->s_v0 = status;

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
    state_PTR old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];

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
    old -> s_v0 = i;

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
    
    devNum = TERM0DEV + (ID - 1);
    old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];
    terminal = &(devReg -> devreg[devNum]);
    
    SYSCALL(PASSEREN, (int)&mutexArray[TERMREADSEM + (ID -1)], 0, 0);
    
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

        if((status & 0xFF) != 5) /* 5 is from blue book */
        {
            PANIC();
        }

        addr++;
    }
    
    old->s_v0 = i;

    SYSCALL(VERHOGEN, (int)&mutexArray[TERMREADSEM + (ID -1)], 0, 0);

}

void writeTerminal(char* virtAddr, int len, int ID)
{
    unsigned int status;
    int i = 0;
    int devNum = TERM0DEV + (ID - 1);
    devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
    device_t* terminal;
    terminal = &(devReg -> devreg[devNum]);
    state_PTR old = (state_PTR) &uProcs[ID-1].uProc_states[SYSTRAP][OLD];

    SYSCALL(PASSEREN, (int)&mutexArray[TERMSEMSTART + (ID -1)], WRITETERM, 0);

    while(i < len)
    {
        Interrupts(FALSE);
        terminal -> t_transm_command = TRANSCHAR | (((unsigned int) *virtAddr) << SHIFT_CHAR);
        status = SYSCALL(WAITIO, TERMINT, (ID -1), WRITETERM);
        Interrupts(TRUE);

        if((status & 0XFF) != TRANSMITCHAR)
        {
            PANIC();
        }
        virtAddr++;
        i++;
    }

    old -> s_v0 = i;

    SYSCALL(VERHOGEN, (int)&mutexArray[TERMSEMSTART + (ID -1)], 0, 0);
}

HIDDEN int spinTheBottle() 
{
    static int next = 0;
    next = (next + 1) % SWAPSIZE;
    return (next);
}

void readWriteBacking(int cylinder, int sector, int head, 
									int isRead, memaddr address)
{
	
	/*Local Variable Declarations*/
	unsigned int diskStatus;
	devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;
	device_t* diskDevice = &(devReg->devreg[DISK0]);
	
	/*Error case*/
	if(isRead != DISK_WRITEBLK && isRead != DISK_READBLK){
		PANIC();
	}
	
	/*Gain mutex on backing store*/
	SYSCALL(PASSEREN, (int)&mutexArray[DISK0], 0, 0);
	
	/*Perform atomic operation and seek to correct cylinder*/
	Interrupts(FALSE);
	
	diskDevice->d_command = ((cylinder-1) << SHIFT_SEEK) | DISK_SEEKCYL;
	diskStatus = SYSCALL(WAITIO, DISKINT, DISK0, 0);
	Interrupts(TRUE);
			
	/*If the device finished seeking...*/
	if(diskStatus == READY){
		
		Interrupts(FALSE);
		/*Initialize where to read from and set command to write*/
		diskDevice->d_data0 = address;
		diskDevice->d_command = (head << SHIFT_HEAD) | 
							(sector << SHIFT_SECTOR) | isRead;
														   
		/*Wait for disk write I/O*/
		diskStatus = SYSCALL(WAITIO, DISKINT, DISK0, 0);
		Interrupts(TRUE);
	}
	
	/*Release mutex on backing store*/
	SYSCALL(VERHOGEN, (int)&mutexArray[DISK0], 0, 0);

}
