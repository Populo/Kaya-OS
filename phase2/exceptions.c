#include "../h/types.h"
#include "../h/const.h"


#include "../e/pcb.e"

#include "../e/exceptions.e"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;

HIDDEN void sysCreate(state_PTR state);
HIDDEN void sysTerminate();
HIDDEN void sysVerhogen(int *semAdd);
HIDDEN void sysPasseren(int *semAdd);
HIDDEN void sysCPUTime(state_PTR state);
HIDDEN void sysWaitClock();
HIDDEN void sysWaitIO(int interruptLine, int deviceNum, int isTerminal);
HIDDEN void pullUpAndDie(int type, state_PTR old, state_PTR new);

void copyState(state_PTR old, state_PTR new);


void sysCallHandler()
{
    /* pc = pc+4 */ /* we need this */
    state_PTR old;
    int sysCall;

    old  = (state_PTR) SYSCALLOLDAREA;
    
    old -> s_status; /* user mode or kernel mode */
    sysCall = old -> s_a0; /* which syscall was executed */

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
            sysVerhogen(old -> s_a1);
            break;
        case PASSEREN:
            sysPasseren(old -> s_a1);
            break;
        case SPECIFY_EXCEPTION_STATE_VECTOR:
            pullUpAndDie(old -> s_a1, old -> s_a2, old -> s_a3);
            break;
        case GET_CPU_TIME:
            sysCPUTime(old);
            break;
        case WAIT_FOR_CLOCK:
            sysWaitClock();
            break;
        case WAIT_FOR_IO_DEVICE:
            sysWaitIO(old -> s_a1, old -> s_a2, old -> s_a3);
            break;
        default: /* handle 9-255 */
            PASSUPORDIE();
            break;
    }
}
else /* User mode */
{
    state_PTR oldTrap = (state_PTR)PGMTRAPOLDAREA;
    copyState((state_PTR)SYSCALLOLDAREA, oldTrap);
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
        insertChild(currentProcess, newProc);
        state -> s_v0 = 0;
    }
    else
    {
        state -> s_v0 = -1;
    }
    /* LDST  we need this at some point on a lot of the syscalls */
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
                death = death -> pcb_parent;
                freePcb(removeChild(death));
                --processCount;           
            }
        }
    }
}

void sysVerhogen(int *semAdd)
{

}

void sysPasseren(int *semAdd)
{

}

void sysCPUTime(state_PTR state)
{
    state -> s_v0 = currentProcess -> cpu_time;
}

void sysWaitClock()
{

}

void sysWaitIO(int interruptLine, int deviceNum, int isTerminal)
{

}

HIDDEN void pullUpAndDie(int type, state_PTR old, state_PTR new)
{
    switch(type)
    {
        case TLB:
            if(currentProcess -> oldTLB != NULL)
            {
                sysTerminate();
            }
            break;
        case PGMTRAP: 
            if(currentProcess -> oldPGM != NULL)
            {
                sysTerminate();
            }              
            break;
        case SYSBP: 
            if(currentProcess -> oldSys != NULL)
            {
                sysTerminate();
            }
            break;
        default:
            sysTerminate();
            break;
    }
    if(currentProcess != NULL)
    {
        copyState(old, new);
        state_PTR newLocation = type == TLB ? currentProcess -> newTLB : type == PGMTRAP ? currentProcess -> newPGM : type == SYSBP ? currentProcess -> newSys : NULL;
        LDST(&newLocation);
    }
}


void pbgTrapHandler()
{

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
