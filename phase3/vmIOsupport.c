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