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


/*    카야     */

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
                pullUpAndDie((int) old -> s_a1);
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

/* 김정은 다시 공격하다 */
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
    int semAdd = old -> s_a1;
    semAdd++;
    if(semAdd <= 0)
    {
        new = removeBlocked(semAdd);
        new -> pcb_semAdd = NULL;

        insertProcQ(&(readyQueue), new);
        
    } 
}

void sysPasseren(state_PTR old)
{
    int semAdd = old -> s_a1;
    semAdd--;
    if(semAdd < 0)
    {
        STCK(TODStopped);
        current = TODStopped - TODStarted;
        currentProcess -> pcb_time = currentProcess -> pcb_time + current;
        insertBlocked(semAdd, currentProcess);
        currentProcess = NULL;
        scheduler();
    }
}

void sysSpecifyException(state_PTR caller)
{
    int type = caller -> s_a1;
    state_PTR old = (state_PTR) caller -> s_a2;
    state_PTR new = (state_PTR) caller -> s_a3;

    switch(type)
    {
        case TLBTRAP:
            if(currentProcess -> pcb_states[TLBTRAP][NEW] != NULL)
            {
                sysTerminate(); /* 김정은 다시 공격하다 */
            }
            currentProcess -> pcb_states[TLBTRAP][NEW] = (state_PTR) new;
            currentProcess -> pcb_states[TLBTRAP][OLD] = (state_PTR) old;
            break;
        case PROGTRAP:
            if(currentProcess -> pcb_states[PGMTRAP][NEW] != NULL)
            {
                sysTerminate(); /* 김정은 다시 공격하다 */
            }
            currentProcess -> pcb_states[PGMTRAP][NEW] = (state_PTR) new;
            currentProcess -> pcb_states[PGMTRAP][OLD] = (state_PTR) old;
            break;
        case SYSTRAP:
            if(currentProcess -> pcb_states[SYSTRAP][NEW] != NULL)
            {
                sysTerminate(); /* 김정은 다시 공격하다 */
            }
            currentProcess -> pcb_states[SYSTRAP][NEW] = (state_PTR) new;
            currentProcess -> pcb_states[SYSTRAP][OLD] = (state_PTR) old;
            break;
        default:
            sysTerminate(); /* 김정은 다시 공격하다 */
            break; /* fuck you */
    }
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
    debugC(4097);

    if(interruptLine < DISKINT || interruptLine > TERMINT)
    {
        debugC(4107);
        sysTerminate(); /* 김정은 다시 공격하다 */
    }  
    debugC(4098);
    if(interruptLine == TERMINT && isRead == TRUE)
    {
        debugC(4108);
        interruptLine++;
    }
    
    debugC(4099);
    index = (int *)(DEVPERINT * (interruptLine - DEVNOSEM) + deviceNum);
    debugC(4100);

      
    semAdd = &(sem[index]);
    --(*semAdd);
    debugC(4101);
    if((*semAdd) < 0)
    {
        debugC(4102);
        insertBlocked(&sem[*semAdd], currentProcess);
        debugC(4103);
        copyState(old, &(currentProcess -> pcb_s));
        debugC(4104);
        softBlockCount++;
        debugC(4105);
        scheduler();
    }
    else{
        currentProcess -> pcb_s.s_v0 = sem[index];
        debugC(4109);
        LDST(old);
    }
    debugC(4106);

}

void pullUpAndDie(int type)
{
    debugH(type);
    debugQ(1);
    if(type == TLBTRAP)
    {
        if(currentProcess -> pcb_states[TLBTRAP][NEW] != NULL)
        {
            debugH(2);
            copyState((state_PTR) TBLMGMTOLDAREA, currentProcess -> pcb_states[TLBTRAP][OLD]);
            copyState(currentProcess -> pcb_states[TLBTRAP][NEW], &(currentProcess -> pcb_s));
            LDST((state_PTR) SYSCALLOLDAREA);
        }
        else
        {
            debugH(5);
            sysTerminate(); /* 김정은 다시 공격하다 */
            scheduler();
        }
    }
    else if(type == PROGTRAP) 
    {
        if(currentProcess -> pcb_states[PGMTRAP][NEW] != NULL)
        {
            debugH(2);
            copyState((state_PTR) PGMTRAPOLDAREA, currentProcess -> pcb_states[PGMTRAP][OLD]);
            copyState(currentProcess -> pcb_states[PGMTRAP][NEW], &(currentProcess -> pcb_s));
            LDST((state_PTR) SYSCALLOLDAREA);
        }
        else
        {
            debugH(5);
            sysTerminate(); /* 김정은 다시 공격하다 */
            scheduler();
        }
    }
    else if(type == SYSTRAP) 
    {
        if(currentProcess -> pcb_states[SYSTRAP][NEW] != NULL)
        {
            debugH(2);
            copyState((state_PTR) SYSCALLOLDAREA, currentProcess -> pcb_states[SYSTRAP][OLD]);
            copyState(currentProcess -> pcb_states[SYSTRAP][NEW], &(currentProcess -> pcb_s));
            LDST((state_PTR) SYSCALLOLDAREA);
        }
        else
        {
            debugH(5);
            sysTerminate(); /* 김정은 다시 공격하다 */
            scheduler();
        }          
    }  
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

