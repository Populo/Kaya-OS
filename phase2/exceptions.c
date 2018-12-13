/******************************************************************
 * exceptions.c
 * This file handles Program Traps, Memory Management Exceptions, 
 * and Syscall exceptions. Program traps and Memory Management
 * Exceptions are either killed or passed to the process's 
 * appropriate handler.
 * 
 * When a syscall happens there are multiple cases with multiple
 * sub cases:
 *		syscall is between 1 and 8:
 * 			process is in user mode:
 * 				call Program Trap with Reserved Instruction
 * 			process is in kernel mode:
 * 				handle the syscall appropriately
 * 		syscall is not in the valid range:
 * 			pass up or die on syscall
 * 
 * Written by Chris Staudigel and Grant Stapleton 
 *****************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/asl.e"
#include "../e/pcb.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"

#include "/usr/local/include/umps2/umps/libumps.e"

extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int softBlockCount;
extern int processCount;
extern int sem[TOTALSEM];
extern cpu_t TODStarted;



cpu_t currentTOD;


/* Handle Process Exception */
HIDDEN void pullUpAndDie(int type);
/* Load State */
void putALoadInMeDaddy(state_PTR state);
/* Copy State to New Location */
void copyState(state_PTR old, state_PTR new);
/* kill the process */
HIDDEN void executeOrderSixtySix(pcb_PTR proc);

/* SYS 1 - Create Process */
HIDDEN void sysCreate(state_PTR state);
/* SYS 2 - Terminate Process */
HIDDEN void sysSendToNorthKorea();
/* SYS 3 - Unblock Process */
HIDDEN void sysSignal(state_PTR state);
/* SYS 4 - Block Process */
HIDDEN void sysWait(state_PTR state);
/* SYS 5 - Specify Handler For Exception */
HIDDEN void sysBYOL(state_PTR state);
/* SYS 6 - Get CPU Time */
HIDDEN void sysGetCPUTime(state_PTR state);
/* SYS 7 - Wait For Clock */
HIDDEN void sysWaitForClock(state_PTR state);
/* SYS 8 - Wait For IO */
HIDDEN void sysGoPowerRangers(state_PTR state);

void debugC(int i)
{
	int j;
	j = i;
}



/******************************************************************
 * pbgTrapHandler
 * 
 * Handles Program Trap Exceptions by calling pullUpAndDie
 * on the program trap state
 * 
 *****************************************************************/
void pbgTrapHandler()
{
	pullUpAndDie(PROGTRAP);
}	

/******************************************************************
 * tlbTrapHandler
 * 
 * Handles Memory Management Exceptions by calling pullUpAndDie
 * on the memory management state
 * 
 *****************************************************************/
void tlbTrapHandler()
{
	pullUpAndDie(TLBTRAP);
}

/******************************************************************
 * sysCallHandler
 * 
 * Handles syscalls with the multiple applicable cases.
 * cases:
 * 		syscall is between 1 and 8:
 * 			process is in user mode:
 * 				call Program Trap with Reserved Instruction
 * 			process is in kernel mode:
 * 				handle the syscall appropriately
 * 		syscall is not in the valid range:
 * 			pass up or die on syscall
 * 
 * 
 *****************************************************************/
void sysCallHandler()
{
	state_PTR pgmOld;
	state_PTR state = (state_PTR) SYSCALLOLDAREA;
	int call = state -> s_a0;
	unsigned int temp;

	if((call >= CREATE_PROCESS && call <= WAITIO)) /* valid syscall */
	{
		if((state -> s_status & KUON) != ALLOFF) /* user mode */
		{
			pgmOld = (state_PTR) PGMTRAPOLDAREA;
			copyState(state, pgmOld);
			/* set cause to priviledged insruction, shift by 2 bits */
			pgmOld -> s_cause = RI << 2;
			/* call a program trap */
			pbgTrapHandler();
		}
	}
	else
	{
		/* pull up and die on systrap */
		pullUpAndDie(SYSTRAP);
	}

	/* increment program counter by 4 once, not twice */
	state -> s_pc = state -> s_pc + 4;

	/* route syscall appropriately */
	switch (call)
	{
		case CREATE_PROCESS:
			sysCreate(state);
			break;
		case TERMINATE_PROCESS:
			sysSendToNorthKorea();
			break;
		case VERHOGEN:
			sysSignal(state);
			break;
		case PASSEREN:
			sysWait(state);
			break;
		case SESV:
			sysBYOL(state);
			break;
		case GETTIME:
			sysGetCPUTime(state);
			break;
		case WAITCLOCK:
			sysWaitForClock(state);
			break;
		case WAITIO:
			sysGoPowerRangers(state);
			break;
	}
	debugA(20);
	/* if syscall doesnt redirect, load the state */
	putALoadInMeDaddy(state);
}

/******************************************************************
 * sysCreate
 * param: state_PTR state
 * SYSCALL 1: Create Process
 * 
 * This method will try to allocate a new job.  If it is successful
 * it will insert the new job into the ready queue, make it a child
 * of the process that made the syscall, make the state the same as 
 * the caller, and go back to the job that made the syscall.
 * 
 * If there are no free jobs, we return a failure code and return
 * to the calling process
 *****************************************************************/
void sysCreate(state_PTR state)
{
	pcb_PTR newProcess = allocPcb();

	if (!emptyProcQ(newProcess)) /* there was a free pcb */
	{
		++processCount;
		/* insert to the ready queue */
		insertChild(currentProcess, newProcess);
		/* make it a child of the calling process */
		insertProcQ(&readyQueue, newProcess);
		/* duplicate the state */
		copyState((state_PTR)state -> s_a1, &(newProcess -> pcb_state));

		/* success code */
		state -> s_v0 = SUCCESS;
	}
	else /* there was not a free pcb */
	{
		/* failure code */
		state -> s_v0 = FAILURE;
	}
}

/******************************************************************
 * sysSendToNorthKorea
 * SYSCALL 2: Terminate
 * 
 * This process calls our recursive helper method to finish the 
 * job. Once the job is done, call scheduler and get a new job.
 *****************************************************************/
void sysSendToNorthKorea()
{
	/* call recursive helper method */
	executeOrderSixtySix(currentProcess);

	/* get a new job */
	scheduler();
}

/******************************************************************
 * sysSignal
 * param: state_PTR state
 * SYSCALL 3
 * 
 * Performs a V operation on the given semaphore. If the semaphore
 * is less than or equal to zero, it unblocks the process and adds
 * it to the readyQueue to be run when the scheduler reaches it.
 * After this method we will return to the calling process.
 *****************************************************************/
void sysSignal(state_PTR state)
{
	/* grab semaphore address from state parameter */
	int *mutex = (int *)state -> s_a1;
	/* increment semaphore value */
	++(*mutex);

	if (*mutex <= 0)
	{
		/* remove process from semaphore */
		pcb_PTR process = removeBlocked(mutex);
		/* verify process exists */
		if(process != NULL)
		{
			/* reset semaphore on job */
			process -> pcb_semAdd = NULL;
			/* insert to ready queue */
			insertProcQ(&readyQueue, process);
		}
	}
}

/******************************************************************
 * sysWait
 * param: state_PTR state
 * SYSCALL 4
 * 
 * Performs a P operation on the given semaphore.  If the semaphore
 * is less than zero it blocks the current process on that address.
 *****************************************************************/
void sysWait(state_PTR state)
{
	/* grab semaphore from parameters */
	int *mutex = (int *)state -> s_a1;

	/* decrement value */
	--(*mutex);

	if (*mutex < 0)
	{
		/* update the state on the current process */
		copyState(state, &(currentProcess -> pcb_state));
		/* block process */
		insertBlocked(mutex, currentProcess);
		/* we need a new process */
		scheduler();
	}
}

/******************************************************************
 * sysBYOL (Bring Your own Lawyer)
 * param: state_PTR state
 * SYSCALL 5
 * 
 * This syscall sets up custom handlers for the process when
 * an exception happens.  This method can only be called once
 * per exception per process.  If the process has already made the 
 * call for that exception we kill the process.
 *****************************************************************/
void sysBYOL(state_PTR state)
{
	/* gather parameters */
	int type = (int)state -> s_a1;
	state_PTR old = (state_PTR) state -> s_a2;
	state_PTR new = (state_PTR) state -> s_a3;

	/* location we are looking at */
	/* custom states are stored in a 2d array */
	state_PTR lookingAt = currentProcess -> pcb_states[type][NEW];

	/* if the call has already been made for this process */
	if (lookingAt != NULL)
	{
		/* kill it */
		sysSendToNorthKorea();
	}

	/* set appropriate old state */
	currentProcess -> pcb_states[type][OLD] = old;
	/* set appropriate new state */
	currentProcess -> pcb_states[type][NEW] = new;
}

/******************************************************************
 * sysGetCPUTime
 * param: state_PTR state
 * SYSCALL 6
 * 
 * This syscall returns the current cpu time of the process and
 * stores it in v0
 *****************************************************************/
void sysGetCPUTime(state_PTR state)
{
	/* update process's state to be more current */
	copyState(state, &(currentProcess -> pcb_state));
	/* grab clock */
	STCK(currentTOD);
	cpu_t total;
	/* total time since process started */
	total = currentTOD - TODStarted;
	/* update time in pcb */
	(currentProcess -> pcb_time) = (currentProcess -> pcb_time) + total;
	/* place time in v0 */
	(currentProcess -> pcb_state.s_v0) = (currentProcess -> pcb_time);
	/* store starting clock */
	STCK(TODStarted);
	/* load current process */
	debugA(21);
	putALoadInMeDaddy(&(currentProcess -> pcb_state));
}

/******************************************************************
 * sysWaitForClock
 * param: state_PTR state
 * SYSCALL 7
 * 
 * This syscall performs a P operation on the clock semaphore.
 * If the semaphore is less than zero, it stores the elapsed time,
 * blocks the process, and gets a new job. Otherwise it returns
 * to the running process.
 *****************************************************************/
void sysWaitForClock(state_PTR state)
{
	/* last item in semaphore array is the clock */
	int *semAdd = (int *)&(sem[TOTALSEM - 1]);
	/* decrement semaphore */
	--(*semAdd);

	if (*semAdd < 0)
	{
		/* block process */
		insertBlocked(semAdd, currentProcess);
		/* update the state */
		copyState(state, &(currentProcess -> pcb_state));
		/* increment soft block count */
		++softBlockCount;
	}

	/* get a new job */
	scheduler();
}

/******************************************************************
 * sysGoPowerRangers
 * param: state_PTR state
 * SYSCALL 8
 * 
 * This syscall performs a P operation on the specified device 
 * semaphore. if the semaphore is less than 0 it blocks the process
 * and gets a new job. Otherwise it returns to the current process
 *****************************************************************/
void sysGoPowerRangers(state_PTR state)
{
	/* grab parameters from state */
	int interruptNumber = (int) state -> s_a1;
	int deviceNumber = (int) state -> s_a2;
	int isWrite = (int) state -> s_a3;

	/* appropriate line number */
	int deviceIndex = interruptNumber - DEVNOSEM + isWrite;
	
	/* 8 devices per interrupt */
	deviceIndex = deviceIndex * DEVPERINT;
	
	/* specific device */
	deviceIndex = deviceIndex + deviceNumber;
    
	/* get appropriate semaphore */
	int *mutex;
	mutex = &(sem[deviceIndex]);
	
	/* decrement value */
	--(*mutex);

	if (*mutex < 0)
	{
		/* update the state on the current process */
		copyState(state, &(currentProcess -> pcb_state));
		/* block process */
		insertBlocked(mutex, currentProcess);
		/* increment soft block count */
		++softBlockCount;
		/* we need a new process */
		scheduler();
	}

}

/******************************************************************
 * executeOrderSixtySix
 * param: pcb_PTR proc
 * 
 * Helper method to kill the process and all of it's children
 * 
 * Will recursively kill all of the process's children up to n
 * generations, then will kill the current process and handle 
 * semaphores if the process is blocked.
 *****************************************************************/
void executeOrderSixtySix(pcb_PTR proc)
{
	/* while the process has children */
	while(!emptyChild(proc))
	{
		/* recursively kill children */
		executeOrderSixtySix(removeChild(proc));
	}

	/* we have arrived at the root cause of the genocide */
	if (currentProcess == proc)
	{
		outChild(proc);
	}

	/* if process is not blocked */
	if (proc -> pcb_semAdd == NULL)
	{
		outProcQ(&readyQueue, proc);
	}
	else /* process is blocked */
	{
		int *semAdd = proc -> pcb_semAdd;
		outBlocked(proc);

		/* blocked on device semaphore */
		if (semAdd >= &(sem[0]) && semAdd <= &(sem[TOTALSEM]))
		{
			
			--softBlockCount;
		}
		else /* normal block */
		{
			++*semAdd;
		}
	}

	/* add job to free list and decrement count */
	freePcb(proc);
	processCount--;
}

/******************************************************************
 * pullUpAndDie
 * param: int type
 * 
 * this method will determine which process custom handler to load.
 * If the custom handler has been set it will load that, otherwise
 * it will kill the process and all of the children
 *****************************************************************/
void pullUpAndDie(int type)
{
	state_PTR lookingAt, location;
	/* grab correct state */
	lookingAt = currentProcess -> pcb_states[type][NEW];
	debugC(6969);
	debugC(currentProcess -> pcb_states[type][OLD] -> s_cause >> 2);
	/* handler has not been set */
	if (lookingAt == NULL)
	{
		sysSendToNorthKorea();
	}

	/* grab appropriate state area in low order memory */
	switch(type)
	{
		case TLBTRAP:
			location = (state_PTR) TBLMGMTOLDAREA;
			break;
		case PROGTRAP:
			location = (state_PTR) PGMTRAPOLDAREA;
			break;
		case SYSTRAP:
			location = (state_PTR) SYSCALLOLDAREA;
			break;
		default:
			sysSendToNorthKorea();
	}

	/* copy state to load */
	copyState(location, currentProcess -> pcb_states[type][OLD]);
	
	/* load custom handler */
	debugA(14);
	putALoadInMeDaddy(lookingAt);
}

/******************************************************************
 * putALoadInMeDaddy
 * param: state_PTR state
 * 
 * Helper method to load a state.  That is literally all this does,
 * load the passed state
 *****************************************************************/
void putALoadInMeDaddy(state_PTR state)
{
	LDST(state);
}

/******************************************************************
 * copyState
 * param: state_PTR old, state_PTR new
 * 
 * this method takes the values of the old state and yeets them
 * over into the new state, effectively copying the state.
 *****************************************************************/
void copyState(state_PTR old, state_PTR new)
{
	new -> s_cause = old -> s_cause;
	new -> s_asid = old -> s_asid;
	new -> s_status = old -> s_status;
	new -> s_pc = old -> s_pc;
	int i;
	/* loop through all registers and copy them individually */
	for(i = 0; i < STATEREGNUM; ++i)
	{
		new -> s_reg[i] = old -> s_reg[i];
	}
}
