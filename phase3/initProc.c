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
int mutexArray[MAXSEM];
int sem;
uProc_PTR uProcs[MAXUSERPROC];


/* INIT's KUSEGOS / 2 / 3 page tables. */

HIDDEN int getCurrentASID() {
    return ((getENTRYHI() & GET_ASID) >> SHIFT_ASID);
}

HIDDEN int setASID(int asid) {
    return SET_ASID | (asid << SHIFT_ASID);
}

HIDDEN void toggleInterrupts(int on) {
    unsigned int status = getSTATUS();

    status = on ? 
        (status | IECON) : /* turning on */
        (status & IECOFF); /* turning off */

    setSTATUS(status);
}

void test()
{
    int i;
    int j;
    state_PTR procState;
    state_PTR delayState;
    segTbl_t* segTable;

    kSegOS.header = (PGTBLMAGICNUM << 24) | KSEGSIZE;

    for(i = 0; i < KSEGSIZE; i++)
    {
        kuSegOS.pteTable[i].entryHI = ((0x20000 + i) << ENTRYHISHIFT);
        kuSegOS.pteTable[i].entryLO = ((0x20000 + i) << ENTRYHISHIFT) | DIRTY | GLOBAL | VALID;

    }

    kuseg3.header = (PTEMAGICNO << 24) | 32;
    for(i = 0; i < 32; i++)
    {
        kuSeg3.pteTable[i].entryHI = ((0xC0000 + i) << ENTRYHISHIFT);
        kuseg3.pteTable[i].entryLO = ALLOFF | DIRTY | GLOBAL;
    }

    for(i = 0; i < SWAPSIZE; i++)
    {
        swapPool[i].sw_asid = -1;
        swapPool[i].sw_pte = NULL;
    }

    swapSem = 1;

    for(i = 0; i < MAXSEM; i++)
    {
        mutexArray[i] = 1;
    }

    sem = 0;

    for(i = 0; i < MAXUSERPROC + 1; i++)
    {
        uProcs[i-1] -> uProc_pte.header = (PTEMAGICNO << 24) | 32;

        for(j = 0; j < 24; j++)
        {
            uProcs[i-1] -> uProc_pte.pteTable[j].entryHI = ((0x80000 + j) << ENTRYHISHIFT) | (i << ASIDSHIFT);
            uProcs[i-1] -> uProc_pte.pteTable[j].entryLO = ALLOFF | DIRTY;
        }

        uProcs[i-1] -> uProc_pte.pteTable[KUSEGPTESIZE].entryHI = (0xBFFFF << ENTRYHISHIFT) | (i * ASIDSHIFT);

        segTable = (segTable_t *) (SEGTBLSTART + (i * SEGTBLWIDTH));

        segTable -> ksegOS = &kuSegOS;
        segTable -> kuseg2 = &(uProcs[i-1]. -> uProc_pte);
        segTable -> kuseg3 = &kuSeg3;

        procState -> s_asid = (i << ASIDSHIFT);
        procState -> s_sp = EXECTOP - ((i - 1) * UPROCSTCKSIZE);
        procState -> s_pc = procState -> s_t9 = (memaddr) uProcInit();
        procState -> s_status = ALLOFF | IEON | IMON | LTON;

        uProcs[i-1] -> uProc_semAdd = 0;

        SYSCALL(CREATEPROCESS, procState);
    }

    initADL();
    intAVSL();

    delayState -> s_asid = MAXUSERPROC + 2;
    delayState -> s_sp = EXECTOP - (MAXUSERPROC * UPROCSTCKSIZE);
    delayState -> s_pc = delayState -> s_t9 = (memaddr) delayDaemon();
    delayState -> s_status = ALLOFF | IEON | IMON | LTON;

    SYSCALL(CREATEPROCESS, delayState);

    for(i = 0; i < MAXUSERPROC; i++)
    {
        SYSCALL(PASSEREN, (int)&masterSem);
    }

    SYSCALL(TERMINATEPROCESS);
}

void uProcInit()
{
    /* stuff goes here to set up the procState pc/t9 */

    int asid = getCurrentASID(),
        i,
        /* tape device number */
        deviceNumber = ((TAPEINT - DEVNOSEM) * DEVPERINT) + (asid + 1),
        finished = FALSE;

    devregarea_t *device = (devregarea_t *) RAMBASEADDR;

    memaddr newLocation;

    uProc_PTR uProc = uProcs[asid-1];
    
    state_PTR new;

    /* location is the only difference between these states */
    new -> s_status = ALLOFF | IMON | IEON | LTON | VMON | KUON;
    new -> s_entryHI = (new -> s_entryHI & setASID(asid));

    /* sys 5 the process */

    for (i = 0; i < TRAPTYPES; ++i) 
    {
        switch (i)
        {
            case SYSTRAP:
                newLocation = (memaddr) sysCallHandler();
                break;
            case TLBTRAP:
                newLocation = (memaddr) tlbTrapHandler();
                break;
            case PROGTRAP:
                newLocation = (memaddr) pbgTrapHandler();
        }
        
        new -> s_pc = new -> s_t9 = newLocation;

        SYSCALL(SPECTRAPVEC,                    /* syscall number (5) */
                i,                              /* trap type */
                uProc -> uProc_states[i][OLD],  /* old state */
                new)                            /* new state */
    }

    /* read contents of tape device onto disk0 */

    /* gain mutual exclusion on tape */
    SYSCALL(PASSEREN,                   /* syscall number (4) */
            &mutexArray[deviceNumber])  /* device to P */

    int currentBlock = 0;

    device_t *disk, tape;
    unsigned int diskStatus, tapeStatus;

    disk = &(device -> devreg[DISK0]);
    tape = &(device -> devreg[deviceNumber]);

    diskStatus = tapeStatus = READY;

    /* while tape is ready and we arent out of tape */
    while ((tapeStatus == READY) && !finished) {
        /* turn off interrupts */
        toggleInterrupts(FALSE);

        /* set sector on tape to read */
        tape -> d_data0 = OSCODEEND + ((asid - 1) * PAGESIZE);
        tape -> d_command = READBLK;

        /* execute */
        tapeStatus = SYSCALL(WAITFORIO, /* syscall number (8) */
                            TAPEINT,    /* interrupt line */
                            asid - 1);  /* device number */
        /* turn interrupts back on */
        toggleInterrupts(TRUE);

        /* gain mutual exclusion on disk */
        SYSCALL(PASSEREN,               /* syscall number (4) */
                &mutexArray[DISK0]);    /* device to P */
        
        /* turn off interrupts */
        toggleInterrupts(FALSE);

        /* set sector to look for */
        disk -> d_command = (currentBlock << SHIFT_SEEK) | DISK_SEEKCYL;
        /* execute */
        diskStatus = SYSCALL(WAITFORIO, /* syscall number (8) */
                            DISKINT,    /* interrupt line */
                            DISK0);     /* device number */

        /* turn interrupts back on */
        toggleInterrupts(TRUE);

        /* done reading disk */
        if (diskStatus == READY) {
            /* turn off interrupts */
            toggleInterrupts(FALSE);

            /* set sector to read */
            disk -> d_data0 = OSCODEEND + ((asid - 1) * PAGESIZE);
            disk -> d_command = ((asid - 1) << SHIFT_SECTOR) | DISK_WRITEBLK;

            /* execute */
            diskStatus = SYSCALL(WAITFORIO, /* syscall number (8) */
                                DISKINT,    /* interrupt line */
                                DISK0);     /* device number */

            /* turn interrupts back on */
            toggleInterrupts(TRUE);
        }

        /* release mutual exclusion on disk */
        SYSCALL(VERHOGEN,               /* syscall number (3) */
                &mutexArray[DISK0]);    /* semaphore */

        /* if !EOT & !EOF */
        if (tape -> d_data1 != TAPE_EOB) {
            finished = TRUE;
        }

        currentBlock++;
    }

    /* release mutual exclusion on tape device */
    SYSCALL(VERHOGEN,                   /* syscall number (3) */
            &mutexArray[deviceNumber]); /* semaphore */

    /* new state to load */

    STST(new);

    new -> s_entryHI = (new -> s_entryHI & setASID(asid));
    new -> s_sp = (memaddr) SEG3; /* last page of KUseg2 */
    new -> s_status = ALLOFF | IMON | IEON | VMON /* interrupts on, vm on, user mode */
    new -> s_pc = (memaddr) 0; /* TODO - well known address from start of KUseg2? */

    /* load this new state */
    putALoadInMeDaddy(new);
}

