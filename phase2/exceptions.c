#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"

#include "../e/exceptions.e"

extern cpu_t TODStarted;


cpu_t start;
cpu_t current;

HIDDEN void sysCreate(state_PTR state);
HIDDEN void sysTerminate();
HIDDEN void sysVerhogen(int* semAdd);
HIDDEN void sysPasseren(state_PTR old);
HIDDEN void sysSpecifyException(state_PTR caller);
HIDDEN void sysCPUTime(state_PTR state);
HIDDEN void sysWaitClock(state_PTR old);
HIDDEN void sysWaitIO(state_PTR old);
HIDDEN void pullUpAndDie(int type, state_PTR old);

void copyState(state_PTR old, state_PTR new);

void debugC(int i)
{
    int a;
    a = 10;
}

void pbgTrapHandler()
{
    pullUpAndDie(PGMTRAP, (state_PTR) PGMTRAPOLDAREA);
}

void tlbTrapHandler()
{
    pullUpAndDie(PGMTRAP, (state_PTR) TBLMGMTOLDAREA);
}


void sysCallHandler()
{
    state_PTR old;
    int sysCall;

    old  = (state_PTR) SYSCALLOLDAREA;
    sysCall = old -> s_a0; /* which syscall was executed */
    old -> s_pc = old -> s_pc + 4;
    old -> s_t9 = old -> s_t9 + 4;
    if(old -> s_status & KUON == ALLOFF) /* Kernel mode on */
    {
        switch(sysCall)
        {
            case CREATE_PROCESS:
                sysCreate(old -> s_a1);
                break;
            case TERMINATE_PROCESS:
                sysTerminate();
                break;
            case VERHOGEN:
                sysVerhogen((int*) old -> s_a1);
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
                pullUpAndDie(old -> s_a1, old -> s_a2);
                break;
        }
        LDST(&old);
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
        insertProcQ(readyQueue, newProc);
        insertChild(currentProcess, newProc);
        state -> s_v0 = 0;
    }
    else
    {
        state -> s_v0 = -1; /* const.h custom error defs */
    }
}

void sysTerminate()
{
    pcb_PTR death = currentProcess;
    while(death != NULL)
    {
        if (emptyChild(currentProcess -> pcb_child)) 
        {
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
                if (death -> pcb_semAdd != NULL)
                {
                    removeBlocked(death -> pcb_semAdd);
                }
                death = death -> pcb_parent;
                freePcb(removeChild(death));
                --processCount;           
            }
        }
    }
    scheduler();
}

void sysVerhogen(int* semAdd)
{
    debugC(1);
    pcb_PTR new = mkEmptyProcQ();
    sem[*semAdd]++;
    if(sem[*semAdd] <= 0)
    {
        new = removeBlocked(semAdd);
        if(!emptyProcQ(new))
        {
            insertProcQ(&readyQueue, new);
        }
    } 
}

void sysPasseren(state_PTR old)
{
    debugC(2);
    int* semAdd;
    semAdd = old -> s_a1;
    sem[*semAdd]--;
    if(sem[*semAdd] < 0)
    {
        copyState(old, &currentProcess -> pcb_s);
        insertBlocked(semAdd, currentProcess);
        scheduler();
    }
}

void sysSpecifyException(state_PTR caller)
{
    int type = caller -> s_a1;
    state_PTR old = caller -> s_a2;
    state_PTR new = caller -> s_a3;

    switch(type)
    {
        case TLBTRAP:
            if(currentProcess -> newTLB != NULL)
            {
                sysTerminate();
            }
            currentProcess -> newTLB = (state_PTR) new;
            currentProcess -> oldTLB = (state_PTR) old;
            break;
        case PROGTRAP:
            if(currentProcess -> newPGM != NULL)
            {
                sysTerminate();
            }
            currentProcess -> newPGM = (state_PTR) new;
            currentProcess -> oldPGM = (state_PTR) old;
            break;
        case SYSTRAP:
            if(currentProcess -> newSys != NULL)
            {
                sysTerminate();
            }
            currentProcess -> newSys = (state_PTR) new;
            currentProcess -> oldSys = (state_PTR) old;
            break;
        default:
            sysTerminate();
            break; /* fuck you */
    }
}

void sysCPUTime(state_PTR state)
{
    cpu_t t;
    STCK(t);

    currentProcess -> cpu_time = currentProcess -> cpu_time + (t - start);
    state -> s_v0 = currentProcess -> cpu_time;
    STCK(start);
}

void sysWaitClock(state_PTR old)
{
    int *semAdd = (int *)&(sem[TOTALSEM - 1]); /* final semAdd is timer */
    *semAdd--;
    insertBlocked(semAdd, currentProcess);
    copyState(old, &(currentProcess -> pcb_s));
    softBlockCount++;
    scheduler();
}

void sysWaitIO(state_PTR old)
{
    int semAdd, interruptLine, deviceNum, isTerminal;
    interruptLine = old -> s_a1;
    deviceNum = old -> s_a2;
    isTerminal = old -> s_a3;


    if(interruptLine < DISKINT || interruptLine > TERMINT)
    {
        sysTerminate();
    }  

    if(interruptLine == TERMINT && isTerminal)
    {
        interruptLine++;
    }  
    semAdd = DEVPERINT * (interruptLine - DEVNOSEM) + deviceNum;

    sem[semAdd]--;

    if(sem[semAdd] < 0)
    {
        insertBlocked(&sem[semAdd], currentProcess);
        copyState(old, &(currentProcess -> pcb_s));
        softBlockCount++;
        scheduler();
    }

}

HIDDEN void pullUpAndDie(int type, state_PTR old)
{
    state_PTR newLocation;

    switch(type)
    {
        case TLB:
            if(currentProcess -> newTLB != NULL)
            {
                sysTerminate();
            }
            else
            {
                newLocation = currentProcess -> newTLB;
            }
            break;
        case PGMTRAP: 
            if(currentProcess -> newPGM != NULL)
            {
                sysTerminate();
            }
            else
            {
                newLocation = currentProcess -> newPGM;
            }           
            break;
        case SYSBP: 
            if(currentProcess -> newSys != NULL)
            {
                sysTerminate();
            }
            else
            {
                newLocation = currentProcess -> newSys;
            }
            break;
        default:
            newLocation = NULL;
            sysTerminate();
            break;
    }
    if(currentProcess != NULL)
    {
        copyState(old, newLocation);
        if (newLocation != NULL)
        {
            LDST(&newLocation);
        }
    }
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

