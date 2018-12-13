/* INITPROC go fuck yourself */


#include "../h/const.h"
#include "../h/types.h"

#include "../e/initial.e"
#include "../e/exceptions.e"

#include "../e/initProc.e"
#include "../e/vmIOsupport.e"
#include "../e/adl.e"
#include "../e/avsl.e"

#include "/usr/local/include/umps2/umps/libumps.e"

/* Globals */ 

pteOS_t kuSegOS;
pte_t kuSeg3;
swap_t swapPool[SWAPSIZE];

int swap;
int mutexArray[TOTALSEM];
int masterSem;
uProc_t uProcs[MAXUSERPROC];

extern void putALoadInMeDaddy(state_PTR state);

/* HIDDEN void memcpy(int *to, int *from) {
    copy(to, from);
} */


/* INIT's KUSEGOS / 2 / 3 page tables. */

int getCurrentASID() {
    return (int)((getENTRYHI() & GET_ASID) >> SHIFT_ASID);
}

void test()
{
    int i;
    int j;
    state_t procState;
    state_t delayState;
    segTbl_t* segTable;

    kuSegOS.header = (PTEMAGICNO << SHIFT_MAGIC) | KSEGSIZE;
    
    for(i = 0; i < KSEGSIZE; i++)
    {
        kuSegOS.pteTable[i].entryHI = ((0x20000 + i) << SHIFT_VPN);
        kuSegOS.pteTable[i].entryLO = ((0x20000 + i) << SHIFT_VPN) | DIRTY | GLOBAL | VALID;

    }
    kuSeg3.header = (PTEMAGICNO << SHIFT_MAGIC) | KUSEGSIZE;
    for(i = 0; i < KUSEGSIZE; i++)
    {
        kuSeg3.pteTable[i].entryHI = ((0xC0000 + i) << SHIFT_VPN);
        kuSeg3.pteTable[i].entryLO = ALLOFF | DIRTY | GLOBAL;
    }
    for(i = 0; i < SWAPSIZE; i++)
    {
        swapPool[i].sw_asid = -1;
        swapPool[i].sw_pte = NULL;
    }
    swap = 1;

    for(i = 0; i < TOTALSEM; i++)
    {
        mutexArray[i] = 1;
    }
    masterSem = 0;
    for(i = 1; i < MAXUSERPROC + 1; i++)
    {
        uProcs[i-1].uProc_pte.header = (PTEMAGICNO << SHIFT_MAGIC) | KUSEGSIZE;
        for(j = 0; j < KUSEGSIZE; j++)
        {
            uProcs[i-1].uProc_pte.pteTable[j].entryHI = ((0x80000 + j) << SHIFT_VPN) | (i << SHIFT_ASID);
            uProcs[i-1].uProc_pte.pteTable[j].entryLO = ALLOFF | DIRTY;  
        }
        uProcs[i-1].uProc_pte.pteTable[KUSEGSIZE-1].entryHI = (0xBFFFF << SHIFT_VPN) | (i << SHIFT_ASID);

        segTable = (segTbl_t *) (SEGTBLSTART + (i * SEGTBLWIDTH));

        segTable -> ksegOS = &kuSegOS;
        segTable -> kuseg2 = &(uProcs[i-1].uProc_pte);
        segTable -> kuseg3 = &kuSeg3;

        procState.s_asid = (i << SHIFT_ASID);
        procState.s_sp = EXECTOP - ((i - 1) * UPROCSTCKSIZE);
        procState.s_pc = (memaddr) uProcInit;
        procState.s_t9 = (memaddr) uProcInit;
        procState.s_status = ALLOFF | IEON | IMON | LTON;

        uProcs[i-1].uProc_semAdd = 0;

        SYSCALL(CREATE_PROCESS, (int)&procState, 0, 0);
    }

    initADL();
    initAVSL();

    delayState.s_asid = MAXUSERPROC + 2;
    delayState.s_sp = EXECTOP - (MAXUSERPROC * UPROCSTCKSIZE);
    delayState.s_pc = (memaddr) delayDaemon;
    delayState.s_t9 = (memaddr) delayDaemon;
    delayState.s_status = ALLOFF | IEON | IMON | LTON;

    SYSCALL(CREATE_PROCESS, (int)&delayState, 0, 0);

    for(i = 0; i < MAXUSERPROC; i++)
    {
        SYSCALL(PASSEREN, (int)&masterSem, 0, 0);
    }
    SYSCALL(TERMINATE_PROCESS, 0, 0, 0);
}

void uProcInit()
{
    /* stuff goes here to set up the procState pc/t9 */
    int asid = getCurrentASID(),
        i,
        /* tape device number */
        deviceNumber = ((TAPEINT - DEVNOSEM) * DEVPERINT) + (asid - 1),
        finished = FALSE;

    devregarea_t *device = (devregarea_t *) RAMBASEADDR;

    memaddr newLocation, stackPointer;
    memaddr TLBTOP, PROGTOP, SYSTOP;

    state_PTR new, old;
    
    /* location is the only difference between these states */
    
    /* stack locations */
    PROGTOP = SYSTOP = EXECTOP - ((asid - 1) * UPROCSTCKSIZE);
    TLBTOP = PROGTOP - PAGESIZE;

    /* sys 5 the process */
    for (i = 0; i < TRAPTYPES; ++i) 
    {
        new = &(uProcs[asid-1].uProc_states[i][NEW]);
        old = &(uProcs[asid-1].uProc_states[i][OLD]);

        new->s_status = ALLOFF | IMON | IEON | LTON | VMON;
        new->s_asid = (asid);

        switch (i)
        {
            case SYSTRAP:
                new -> s_pc = (memaddr) vmSysHandler;
                new -> s_t9 = (memaddr) vmSysHandler;
                new -> s_sp = SYSTOP;
                break;
            case TLBTRAP:
                new -> s_pc = (memaddr) vmMemHandler;
                new -> s_t9 = (memaddr) vmMemHandler;
                new -> s_sp = TLBTOP;
                break;
            case PROGTRAP:
                new -> s_pc = (memaddr) vmPrgmHandler;
                new -> s_t9 = (memaddr) vmPrgmHandler;
                new -> s_sp = PROGTRAP;
                break;
        }


        SYSCALL(SESV, i, &old, &new);
    }
    /* read contents of tape device onto disk0 */

    /* gain mutual exclusion on tape */
    SYSCALL(PASSEREN,                   /* syscall number (4) */
            (int)&mutexArray[deviceNumber], 0, 0);  /* device to P */

    int currentBlock = 0;

    device_t *disk;
    device_t *tape;
    unsigned int diskStatus, tapeStatus;

    disk = &(device -> devreg[DISK0]);
    tape = &(device -> devreg[deviceNumber]);

    diskStatus = tapeStatus = READY;

    /* while tape is ready and we arent out of tape */
    while ((tapeStatus == READY) && !finished) {
        /* turn off interrupts */
        Interrupts(FALSE);

        /* set sector on tape to read */
        tape -> d_data0 = OSCODEEND + ((asid - 1) * PAGESIZE);
        tape -> d_command = DISK_READBLK;

        /* execute */
        tapeStatus = SYSCALL(WAITIO, /* syscall number (8) */
                            TAPEINT,    /* interrupt line */
                            asid - 1, 0);  /* device number */
        /* turn interrupts back on */
        Interrupts(TRUE);

        /* gain mutual exclusion on disk */
        SYSCALL(PASSEREN,               /* syscall number (4) */
                (int)&mutexArray[DISK0], 0, 0);    /* device to P */
        
        /* turn off interrupts */
        Interrupts(FALSE);

        /* set sector to look for */
        disk -> d_command = (currentBlock << SHIFT_SEEK) | DISK_SEEKCYL;
        /* execute */
        diskStatus = SYSCALL(WAITIO, /* syscall number (8) */
                            DISKINT,    /* interrupt line */
                            DISK0, 0);     /* device number */

        /* turn interrupts back on */
        Interrupts(TRUE);

        /* done reading disk */
        if (diskStatus == READY) {
            /* turn off interrupts */
            Interrupts(FALSE);

            /* set sector to read */
            disk -> d_data0 = OSCODEEND + ((asid - 1) * PAGESIZE);
            disk -> d_command = ((asid - 1) << SHIFT_SECTOR) | DISK_WRITEBLK;

            /* execute */
            diskStatus = SYSCALL(WAITIO, /* syscall number (8) */
                                DISKINT,    /* interrupt line */
                                DISK0, 0);     /* device number */

            /* turn interrupts back on */
            Interrupts(TRUE);
        }

        /* release mutual exclusion on disk */
        SYSCALL(VERHOGEN,               /* syscall number (3) */
                (int)&mutexArray[DISK0], 0, 0);    /* semaphore */

        /* if !EOT & !EOF */
        if (tape -> d_data1 != TAPE_EOB) {
            finished = TRUE;
        }

        currentBlock++;
    }
    /* release mutual exclusion on tape device */
    SYSCALL(VERHOGEN,                   /* syscall number (3) */
            (int)&mutexArray[deviceNumber], 0, 0); /* semaphore */

    /* new state to load */
    state_PTR new2;
    STST(&new2);
    
    new2->s_asid = (asid << SHIFT_ASID);
    new2->s_sp = SEG3; /* last page of KUseg2 */
    new2->s_status = ALLOFF | IMON | IEON | VMON | KUON | LTON; /* interrupts on, vm on, user mode */
    new2->s_pc = uProcStart; 
    new2->s_t9 = uProcStart; 
    /* load this new state */
    putALoadInMeDaddy(new2);
}


void Interrupts(int amIFucked)
{
    int status = getSTATUS();

    if(amIFucked)
    {
        status = (status | 0x1);
    }
    else
    {
        status = (status & 0xFFFFFFFE);
    }
    setSTATUS(status);
}

void copy(int* from, int* to)
{
    int index = 0;
    while(index < (PAGESIZE / WORDLEN))
    {
        *to = *from;

        from++;
        to++;
        index++;
    }
}
