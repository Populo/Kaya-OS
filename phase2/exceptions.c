#include "../h/const.h"
#include "../h/types.h"

#include "../e/asl.e"
#include "../e/pcb.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"

extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int softBlockCount;
extern int processCount;
extern int sem[TOTALSEM];
extern cpu_t TODStarted;


/* Handle Process Exception */
void pullUpAndDie(int type, state_PTR state);
/* Load State */
void putALoadInMeDaddy(state_PTR state);
/* Copy State to New Location */
void copyState(state_PTR old, state_PTR new);

/* SYS 1 - Create Process */
void sysCreate(state_PTR state);
/* SYS 2 - Terminate Process */
void sysTerminate(state_PTR state);
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
void sysWaitForIO(state_PTR state);



void pbgTrapHandler()
{
	pullUpAndDie(PROGTRAP, (state_PTR) PGMTRAPOLDAREA);
}	

void tlbTrapHandler()
{
	pullUpAndDie(TLBTRAP, (state_PTR) TBLMGMTOLDAREA);

}

void sysCallHandler()
{

	state_PTR state = (state_PTR) SYSCALLOLDAREA;
	int call = state -> s_a0;

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
		pullUpAndDie(SYSTRAP, state);
	}

	state -> s_pc = state -> s_pc + 4;

	switch (call)
	{
		case(CREATE_PROCESS):
			sysCreate(state);
			break;
		case(TERMINATE_PROCESS):
			sysTerminate(state);
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
			sysWaitForClock();
			break;
		case(WAIT_FOR_IO_DEVICE):
			sysWaitForIO(state);
			break;
	}

	putALoadInMeDaddy(state);
}

void sysCreate(state_PTR state)
{
	pcb_PTR newProcess = allocPCB();

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

void sysTerminate(state_PTR state)
{
	/* genocide */
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


	state_PTR lookingAt = currentProcess -> pcb_states[NEW][type];

	if (lookingAt != NULL)
	{
		sysTerminate();
	}

	currentProcess -> pcb_states[OLD][type] = old;
	currentProcess -> pcb_states[NEW][type] = new;

	putALoadInMeDaddy(currentProcess -> pcb_state);
}

void sysGetCPUTime()
{
	STCK(currentTOD);
	int elapsedTime = currentTOD - TODStarted;

	currentProcess -> pcb_time = currentProcess -> pcb_time + elapsedTime;
}

void sysWaitForClock()
{
	/* P in the V */
}

void sysWaitForIO(state_PTR state)
{
	int interruptNumber = (int) state -> s_a1;
	int deviceNumber = (int) state -> s_a2;
	int isRead = (int) state -> s_a3;

	/* appropriate line number */
	int deviceIndex = interruptNumber - DEVNOSEM + isRead;
	
	/* 8 devices per interrupt */
	deviceIndex = deviceIndex * ;
	
	/* specific device */
	deviceIndex = deviceIndex + deviceNumber;

	sem[deviceIndex] = sem[deviceIndex] - 1;

	if (sem[deviceIndex] < 0)
	{
					
	}
}
