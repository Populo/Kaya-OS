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
void sysGetCPUTime();
/* SYS 7 - Wait For Clock */
void sysWaitForClock(state_PTR state);
/* SYS 8 - Wait For IO */
void sysWaitForIO(state_PTR state);



void pbgTrapHandler()
{
	pullUpAndDie(PROGTRAP);
}	

void tlbTrapHandler()
{
	pullUpAndDie(TLBTRAP);

}

void sysCallHandler()
{

	state_PTR state = (state_PTR) SYSCALLOLDAREA;
	int call = state -> s_a0;
	state -> s_pc = state -> s_pc + 4;

	if((call >= CREATE_PROCESS && call <= WAIT_FOR_IO_DEVICE)) /* valid syscall */
	{
		if((state -> s_status & KUON) != ALLOFF) /* user mode */
		{
			state -> s_cause = RI;
			copyState(state, (state_PTR) PGMTRAPOLDAREA);
			pbgTrapHandler();
		}
	}
	else
	{
		pullUpAndDie(SYSTRAP);
	}

	state -> s_pc = state -> s_pc + 4;

	switch (call)
	{
		case(CREATE_PROCESS):
			sysCreate(state);
			break;
		case(TERMINATE_PROCESS):
			sysSendToNorthKorea();
			break;
		case(VERHOGEN):
			sysSignal(state);
			break;
		case(PASSEREN):
			sysWait(state);
			break;
		case(SPECIFY_EXCEPTION_STATE_VECTOR):
			sysBYOL(state);
			break;
		case(GET_CPU_TIME):
			sysGetCPUTime();
			break;
		case(WAIT_FOR_CLOCK):
			sysWaitForClock(state);
			break;
		case(WAIT_FOR_IO_DEVICE):
			sysWaitForIO(state);
			break;
	}

	putALoadInMeDaddy(state);
}

void sysCreate(state_PTR state)
{
	pcb_PTR newProcess = allocPcb();

	if (!emptyProcQ(newProcess)) /* there was a free pcb */
	{
		++processCount;
		insertChild(currentProcess, newProcess);
		insertProcQ(&readyQueue, newProcess);
		copyState((state_PTR)state -> s_a1, newProcess -> pcb_state);

		state -> s_v0 = SUCCESS;
	}
	else /* there was not a free pcb */
	{
		state -> s_v0 = FAILURE;
	}


	putALoadInMeDaddy(state);
}

void sysSendToNorthKorea()
{
	pullAMacMiller(currentProcess);

	scheduler();
}

void sysSignal(state_PTR state)
{
	int *mutex = state -> s_a1;
	++*mutex;

	if (*mutex <= 0)
	{
		/* remove process from semaphore */
		pcb_PTR process = removeBlocked(mutex);
		process -> pcb_semAdd = NULL;
		
		/* insert to readyQ) */
		insertProcQ(&readyQueue, process);

		putALoadInMeDaddy(state);
	}
}

void sysWait(state_PTR state)
{
	int *mutex = state -> s_a1;

	--*mutex;

	if (*mutex < 0)
	{
		copyState(state, currentProcess -> pcb_state);

		/* block the process */
		insertBlocked(mutex, currentProcess);
		currentProcess = NULL;
		
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

	putALoadInMeDaddy(currentProcess -> pcb_state);
}

void sysGetCPUTime()
{
	STCK(currentTOD);
	int elapsedTime = currentTOD - TODStarted;

	currentProcess -> pcb_time = currentProcess -> pcb_time + elapsedTime;
}

void sysWaitForClock(state_PTR state)
{
	/* last item in semaphore array */
	int *semAdd = (int *)&(sem[TOTALSEM - 1]);
	--*semAdd;

	copyState(state, currentProcess -> pcb_state);
	insertBlocked(semAdd, currentProcess);

	++softBlockCount;

	scheduler();
}

void sysWaitForIO(state_PTR state)
{
	int interruptNumber = (int) state -> s_a1;
	int deviceNumber = (int) state -> s_a2;
	int isRead = (int) state -> s_a3;

	/* appropriate line number */
	int deviceIndex = interruptNumber - DEVNOSEM + isRead;
	
	/* 8 devices per interrupt */
	deviceIndex = deviceIndex * DEVPERINT;
	
	/* specific device */
	deviceIndex = deviceIndex + deviceNumber;

	/* decrement sem value */
	sem[deviceIndex] = sem[deviceIndex] - 1;

	if (sem[deviceIndex] < 0)
	{
		copyState(state, currentProcess -> pcb_state);
		insertBlocked(&(sem[deviceIndex]), currentProcess);
		++softBlockCount;

		scheduler();
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
		if (semAdd <= &(sem[0]) && semAdd >= &(sem[TOTALSEM]))
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
	state_PTR lookingAt;
	lookingAt = currentProcess -> pcb_states[type][NEW];

	if (lookingAt == NULL)
	{
		sysSendToNorthKorea();
	}

	copyState(lookingAt, currentProcess -> pcb_state);

	putALoadInMeDaddy(currentProcess -> pcb_state);
}

void putALoadInMeDaddy(state_PTR state)
{
	LDST(state);
}

void copyState(state_PTR old, state_PTR new)
{
	new -> s_cause = old -> s_cause;
	new -> s_asid = old -> s_asid;
	new -> s_status = new -> s_status;
	new -> s_pc = new -> s_pc;
	int i;
	for(i = 0; i < STATEREGNUM; ++i)
	{
		new -> s_reg[i] = old -> s_reg[i];
	}
}