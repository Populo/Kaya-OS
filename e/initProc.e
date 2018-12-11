#ifndef INITPROC
#define INITPROC

uProc_t uProcs[MAXUSERPROC];
int swap;
int mutexArray[TOTALSEM];
int masterSem;
swap_t swapPool[SWAPSIZE]; 

pteOS_t kSegOS;
pte_t kuSeg3;

extern int getCurrentASID();

extern void uProcInit();

#endif
