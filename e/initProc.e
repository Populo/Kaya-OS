#ifndef INITPROC
#define INITPROC

uProc_PTR uProcs[8];
int swapSem;
int mutexArray[MAXPROC];
int masterSem;
swap_t swapPool[SWAPSIZE]; 

pteOS_t kSegOS;
pte_t kuSeg3;

extern int getCurrentASID();

#endif