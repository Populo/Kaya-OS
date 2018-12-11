#ifndef INITPROC
#define INITPROC

uProc_t uProcs[MAXUSERPROC];
int swap;
int mutexArray[MAXPROC];
int masterSem;
swap_t swapPool[SWAPSIZE]; 

pteOS_t kSegOS;
pte_t kuSeg3;

extern int getCurrentASID();

#endif
