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
    state_PTR old;
    cpu_t current;
    cpu_t delay;
    int ID = ((getENTRYhi() & ENTRYMASK) >> ASIDSHIFT);

    old = (state_t *) &uProcs[ID-1].Told_trap[SYSTRAP];

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
            break;
        case PSEMVIRT:
            break;
        case DELAY:
            break;
        case DISK_PUT:
            break;
        case DISK_GET:
            break;
        case WRITEPRINTER:
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
    /* kill shit */
}