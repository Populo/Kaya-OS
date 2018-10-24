#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"

#include "../e/exceptions.e"

extern cpu_t TODStarted;
cpu_t TODStopped;

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



void debugC(int i)
{
    int a;
    a = 10;
}

void debugH(int i)
{
    int a;
    a = 10;
}

void debugQ(int i)
{
    int a;
    a = 10;
}

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
    
}

void sysCreate(state_PTR state)
{
    pcb_PTR newProc = allocPcb();
    if(newProc != NULL)
    {
        ++processCount;
        copyState((state_PTR) state -> s_a1, (state_PTR) &(newProc -> pcb_s));
        insertChild(currentProcess, newProc);
        insertProcQ(readyQueue, newProc);
        state -> s_v0 = SUCCESS;
    }
    else
    {
        state -> s_v0 = FAILURE;
    }
}

void sysTerminate()
{
    pcb_PTR death = currentProcess;
    while(death != NULL)
    {
        if (emptyChild(currentProcess)) 
        {
            outChild(currentProcess);
            freePcb(currentProcess);
            --processCount;
            death = NULL;
            currentProcess = NULL;
        }
        else
        {
            if(!emptyChild(death -> pcb_child))
            {
                death = death -> pcb_child;
            }
            else
            {
                if (death -> pcb_semAdd != NULL) /* probably needs something */
                {
                    removeBlocked(death -> pcb_semAdd);
                    softBlockCount--;
                }
                death = death -> pcb_parent;
                freePcb(removeChild(death));
                --processCount;           
            }
        }
    }
    scheduler();
}

void sysVerhogen(state_PTR old)
{
    pcb_PTR new = NULL;
    int* semAdd = old -> s_a1;
    (*semAdd)++;
    if((*semAdd) <= 0)
    {
        new = removeBlocked(semAdd);
        new -> pcb_semAdd = NULL;

        insertProcQ(&(readyQueue), new);
        
    } 
}

void sysPasseren(state_PTR old)
{
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
}

void sysSpecifyException(state_PTR caller)
{
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
}

void sysCPUTime(state_PTR state)
{
    cpu_t t;
    STCK(t);

    currentProcess -> pcb_time = currentProcess -> pcb_time + (t - TODStarted);
    currentProcess -> pcb_s.s_v0 = currentProcess -> pcb_time;
    STCK(TODStarted);
}

void sysWaitClock(state_PTR old)
{
    int device = sem[TOTALSEM-1];
    device--;
    if(device < 0)
    {
        STCK(TODStopped);
        int total = TODStopped - TODStarted;
        currentProcess -> pcb_time = currentProcess -> pcb_time + total;

        insertBlocked(&(device), currentProcess);
        currentProcess = NULL;
        softBlockCount++;

        scheduler();
    }
    PANIC();
}

void sysWaitIO(state_PTR old)
{
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
}

void pullUpAndDie(int type)
{
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

void copyState(state_PTR old, state_PTR new)
{
    int i;
    new -> s_asid = old -> s_asid;
    new -> s_status = old -> s_status;
    new -> s_pc = old -> s_pc;
    new -> s_cause = old -> s_cause;
    for(i = 0; i<STATEREGNUM; i++)
        new -> s_reg[i] = old -> s_reg[i];
}

