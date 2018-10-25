#include "../h/const.h"
#include "../h/types.h"

#include "../e/asl.e"
#include "../e/pcb.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"

#include "/usr/local/include/umps2/umps/libumps.e"

<<<<<<< HEAD
cpu_t current;

HIDDEN void sysCreate(state_PTR state);
HIDDEN void sysTerminate();
void sysVerhogen(state_PTR old);
HIDDEN void sysPasseren(state_PTR old);
HIDDEN void sysSpecifyException(state_PTR caller);
HIDDEN void sysCPUTime(state_PTR state);
HIDDEN void sysWaitClock(state_PTR old);
HIDDEN void sysWaitIO(state_PTR old);

void pullUpAndDie(int type);
void copyState(state_PTR old, state_PTR new);
=======
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
>>>>>>> exceptions



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
	pcb_PTR newProcess = allocPCB();

	if (!emptyProcQ(newProcess)) /* there was a free pcb */
	{
		++processCount;
		insertChild(currentProcess, newProcess);
		insertProcQ(&readyQueue, newProcess);
		copyState((state_PTR)state -> s_a1, newProcess -> pcb_state);

<<<<<<< HEAD
void sysCallHandler()
{
    state_PTR old;
    int sysCall;

    old  = (state_PTR) SYSCALLOLDAREA;
    sysCall = old -> s_a0; /* which syscall was executed */
    old -> s_pc = old -> s_pc + 4;
    old -> s_t9 = old -> s_t9 + 4;
    debugC(15);
    if((old -> s_status & KUON) == ALLOFF) /* Kernel mode on */
    {
        debugC(32);
        switch(sysCall)
        {
            case CREATE_PROCESS:
                sysCreate((state_PTR)old -> s_a1);
                break;
            case TERMINATE_PROCESS:
                sysTerminate();
                break;
            case VERHOGEN:
                sysVerhogen(old);
                break;
            case PASSEREN:
                sysPasseren(old);
                break;
            case SPECIFY_EXCEPTION_STATE_VECTOR:
                sysSpecifyException(old);
                break;
            case GET_CPU_TIME:
                sysCPUTime(old);
                break;
            case WAIT_FOR_CLOCK:
                sysWaitClock(old);
                break;
            case WAIT_FOR_IO_DEVICE:
                sysWaitIO(old);
                break;
            default: /* handle 9-255 */
                pullUpAndDie(SYSTRAP);
                break;
        }
        debugC(43);
        LDST(old);
    }
    else /* User mode */
    {
        state_PTR oldTrap = (state_PTR)PGMTRAPOLDAREA;
        copyState(old, oldTrap);
        oldTrap -> s_cause = RI;
        pbgTrapHandler();
    }
    
=======
		state -> s_v0 = SUCCESS;
	}
	else /* there was not a free pcb */
	{
		state -> s_v0 = FAILURE;
	}


	putALoadInMeDaddy(state);
>>>>>>> exceptions
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
<<<<<<< HEAD
    pcb_PTR new = NULL;
    int* semAdd = old -> s_a1;
    (*semAdd)++;
    if((*semAdd) <= 0)
    {
        new = removeBlocked(semAdd);
        new -> pcb_semAdd = NULL;

        insertProcQ(&(readyQueue), new);
        
    } 
=======
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

>>>>>>> exceptions
}


void sysBYOL(state_PTR state)
{
<<<<<<< HEAD
    int* semAdd = old -> s_a1;
    (*semAdd)--;
    if((*semAdd) < 0)
    {
        STCK(TODStopped);
        current = TODStopped - TODStarted;
        currentProcess -> pcb_time = currentProcess -> pcb_time + current;
        copyState(old, &(currentProcess -> pcb_s));
        insertBlocked(semAdd, currentProcess);
        currentProcess = NULL;
        scheduler();
    }
=======
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
>>>>>>> exceptions
}

void sysGetCPUTime()
{
<<<<<<< HEAD
    int type = (int) caller -> s_a1;
    state_PTR old = (state_PTR) caller -> s_a2;
    state_PTR new = (state_PTR) caller -> s_a3;

    if (currentProcess -> pcb_states[type][NEW] != NULL)
    {
        sysTerminate();
        scheduler();
    }
    
    currentProcess -> pcb_states[type][NEW] = (state_PTR) new;
    currentProcess -> pcb_states[type][OLD] = (state_PTR) old;
=======
	STCK(currentTOD);
	int elapsedTime = currentTOD - TODStarted;

	currentProcess -> pcb_time = currentProcess -> pcb_time + elapsedTime;
>>>>>>> exceptions
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
		insertBlocked((int *)&(sem[deviceIndex]), currentProcess);
		++softBlockCount;

		scheduler();
	}
}

void pullAMacMiller(pcb_PTR proc)
{
<<<<<<< HEAD
    int *semAdd;
    int interruptLine, deviceNum, isRead, index;
    interruptLine = old -> s_a1;
    deviceNum = old -> s_a2;
    isRead = old -> s_a3;
    cpu_t total;
    if(interruptLine < DISKINT || interruptLine > TERMINT)
    {
        
        sysTerminate(); 
    }  
    index = (int *)(DEVPERINT * (interruptLine - DEVNOSEM) + deviceNum);
    if(interruptLine == TERMINT && isRead == FALSE)
    {
        
        index = index + DEVPERINT;
    }  
    sem[index] = sem[index] - 1;
    debugQ(4);
    if(sem[index] < 0)
    {
        debugQ(sem[index]);
        STCK(TODStopped);
        total = TODStopped - TODStarted;
        currentProcess -> pcb_time = currentProcess -> pcb_time + total;
        copyState(old, &(currentProcess -> pcb_s));
        currentProcess -> pcb_semAdd = &(sem[index]);
	insertBlocked(&(sem[index]), currentProcess);
        currentProcess = NULL;
        softBlockCount++;
        scheduler();
    }
    else{
        currentProcess -> pcb_s.s_v0 = sem[index];
        LDST(old);
    }
=======
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
>>>>>>> exceptions
}

void pullUpAndDie(int type)
{
<<<<<<< HEAD
    debugH(type);
    
    state_PTR location;

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
            location = NULL;
            break;
    }

    if (location == NULL || currentProcess -> pcb_states[type][NEW] == NULL)
    {
        sysTerminate();
        scheduler();
    }


    copyState(location, currentProcess -> pcb_states[type][OLD]);
    copyState(currentProcess -> pcb_states[type][NEW], &(currentProcess -> pcb_s));
    LDST(&(currentProcess -> pcb_s));

    debugH(16);      
    
}
=======
	state_PTR lookingAt;
	lookingAt = currentProcess -> pcb_states[type][NEW];
>>>>>>> exceptions

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