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
    int ID = ((getENTRYhi() & ENTRYMASK) >> ASIDSHIFT);
    int *semAdd;

    old = (state_t *) &uProcs[ID-1].Told_trap[SYSTRAP];

    callNumber = old -> s_a0;
    /* P, V, delay, getTOD, and death are all done*/
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
    LDSTS(old);
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