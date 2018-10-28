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
void pullUpAndDie(int type);
/* Load State */
void putALoadInMeDaddy(state_PTR state);
/* Copy State to New Location */
void copyState(state_PTR old, state_PTR new);
/* kill the process */
void pullAMacMiller(pcb_PTR proc);

/* SYS 1 - Create Process */
void sysCreate(state_PTR state);
/* SYS 2 - Terminate Process */
void sysSendToNorthKorea();
/* SYS 3 - Unblock Process */
void sysSignal(state_PTR state);
/* SYS 4 - Block Process */
void sysWait(state_PTR state);
/* SYS 5 - Specify Handler For Exception */
void sysBYOL(state_PTR state);
/* SYS 6 - Get CPU Time */
void sysGetCPUTime(state_PTR state);
/* SYS 7 - Wait For Clock */
void sysWaitForClock(state_PTR state);
/* SYS 8 - Wait For IO */
void sysGoPowerRangers(state_PTR state);



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

	if((call >= CREATE_PROCESS && call <= WAIT_FOR_IO_DEVICE)) /* valid syscall */
	{
		if((state -> s_status & KUON) != ALLOFF) /* user mode */
		{
			pgmOld = (state_PTR) PGMTRAPOLDAREA;
			copyState(state, pgmOld);
			/* set cause to priviledged insruction */
			temp = (pgmOld -> s_cause) & ~(0xFF);
			(pgmOld -> s_cause) = (temp | (10 << 2));
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
		case SPECIFY_EXCEPTION_STATE_VECTOR:
			sysBYOL(state);
			break;
		case GET_CPU_TIME:
			sysGetCPUTime(state);
			break;
		case WAIT_FOR_CLOCK:
			sysWaitForClock(state);
			break;
		case WAIT_FOR_IO_DEVICE:
			sysGoPowerRangers(state);
			break;
	}

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
	pullAMacMiller(currentProcess);

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
		/* copy calling state to process's state */
		copyState(state, &(currentProcess -> pcb_state));
		/* block process */
		insertBlocked(mutex, currentProcess);
		/* we need a new process */
		scheduler();
	}
}


void sysBYOL(state_PTR state)
{
	int type = (int)state -> s_a1;
	state_PTR old = (state_PTR) state -> s_a2;
	state_PTR new = (state_PTR) state -> s_a3;


	state_PTR lookingAt = currentProcess -> pcb_states[type][NEW];

	if (lookingAt != NULL)
	{
		sysSendToNorthKorea();
	}

	currentProcess -> pcb_states[type][OLD] = old;
	currentProcess -> pcb_states[type][NEW] = new;

	putALoadInMeDaddy(state);
}

void sysGetCPUTime(state_PTR state)
{
	copyState(state, &(currentProcess -> pcb_state));
	STCK(currentTOD);
	cpu_t total;
	total = currentTOD - TODStarted;
	(currentProcess -> pcb_time) = (currentProcess -> pcb_time) + total;
	(currentProcess -> pcb_state.s_v0) = (currentProcess -> pcb_time);
	STCK(TODStarted);
	putALoadInMeDaddy(&(currentProcess -> pcb_state));
}

void sysWaitForClock(state_PTR state)
{
	/* last item in semaphore array */
	int *semAdd = (int *)&(sem[TOTALSEM - 1]);
	--(*semAdd);
	insertBlocked(semAdd, currentProcess);
	copyState(state, &(currentProcess -> pcb_state));
	++softBlockCount;
	scheduler();
}

void sysGoPowerRangers(state_PTR state)
{
	int interruptNumber = (int) state -> s_a1;
	int deviceNumber = (int) state -> s_a2;
	int isWrite = (int) state -> s_a3;
	/* appropriate line number */
	int deviceIndex = interruptNumber - DEVNOSEM + isWrite;
	
	/* 8 devices per interrupt */
	deviceIndex = deviceIndex * DEVPERINT;
	
	/* specific device */
	deviceIndex = deviceIndex + deviceNumber;
    int *semADD;
	semADD = &(sem[deviceIndex]);
	/* decrement sem value */
	--*semADD;

	if ((*semADD) < 0)
	{
		insertBlocked(semADD, currentProcess);	
		copyState(state, &(currentProcess -> pcb_state));
		++softBlockCount;

		scheduler();
	}
	else{/* 
		currentProcess -> pcb_state.s_v0 = SUCCESS; */
		putALoadInMeDaddy(state);
	}
}

void pullAMacMiller(pcb_PTR proc)
{
	/* while the process has children */
	while(!emptyChild(proc))
	{
		/* recursively kill children */
		pullAMacMiller(removeChild(proc));
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

	freePcb(proc);
	processCount--;
}

void pullUpAndDie(int type)
{
	state_PTR lookingAt, location;
	lookingAt = currentProcess -> pcb_states[type][NEW];


	if (lookingAt == NULL)
	{
		sysSendToNorthKorea();
	}

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

	copyState(location, currentProcess -> pcb_states[type][OLD]);
	

	putALoadInMeDaddy(lookingAt);
}

void putALoadInMeDaddy(state_PTR state)
{
	LDST(state);
}

void copyState(state_PTR old, state_PTR new)
{
	new -> s_cause = old -> s_cause;
	new -> s_asid = old -> s_asid;
	new -> s_status = old -> s_status;
	new -> s_pc = old -> s_pc;
	int i;
	for(i = 0; i < STATEREGNUM; ++i)
	{
		new -> s_reg[i] = old -> s_reg[i];
	}
}


