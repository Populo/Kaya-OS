/* INITPROC go fuck yourself */


#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
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
Tproc_t uProcs[MAXUSERPROC];


/* INIT's KUSEGOS / 2 / 3 page tables. */


void test()
{
    int i;
    int j;
    state_t procState;
    state_t delayState;
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
        uProcs[i-1].Te_pte.header = (PTEMAGICNO << 24) | 32;

        for(j = 0; j < 24; j++)
        {
            uProcs[i-1].Tp_pte.pteTable[j].entryHI = ((0x80000 + j) << ENTRYHISHIFT) | (i << ASIDSHIFT);
            uProcs[i-1].Tp_pte.pteTable[j].entryLO = ALLOFF | DIRTY;
        }

        uProcs[i-1].Tp_pte.pteTable[KUSEGPTESIZE].entryHI = (0xBFFFF << ENTRYHISHIFT) | (i * ASIDSHIFT);

        segTable = (segTable_t *) (SEGTBLSTART + (i * SEGTBLWIDTH));

        segTable -> ksegOS = &kuSegOS;
        segTable -> kuseg2 = &(uProcs[i-1].Tp_pte);
        segTable -> kuseg3 = &kuSeg3;

        procState.s_asid = (i << ASIDSHIFT);
        procState.s_sp = EXECTOP - ((i - 1) * UPROCSTCKSIZE);
        procState.s_pc = procState.s_t9 = (memaddr) uProcInit();
        procState.s_status = ALLOFF | IEON | IMON | LTON;

        uProcs[i-1].Tp_sem = 0;

        SYSCALL(CREATEPROCESS, (int)&procState, 0, 0);
    }

    initADL();
    intAVSL();

    delayState.s_asid = MAXUSERPROC + 2;
    delayState.s_sp = EXECTOP - (MAXUSERPROC * UPROCSTCKSIZE);
    delayState.s_pc = delayState.s_t9 = (memaddr) delayDaemon();
    delayState.s_status = ALLOFF | IEON | IMON | LTON;

    SYSCALL(CREATEPROCESS, (int)&delayState, 0, 0);

    for(i = 0; i < MAXUSERPROC; i++)
    {
        SYSCALL(PASSEREN, (int)&masterSem, 0, 0);
    }

    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    
}

void uProcInit()
{
    /* stuff goes here to set up the procState pc/t9 */

    
}

