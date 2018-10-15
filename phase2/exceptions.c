#include "../h/types.h"
#include "../h/const.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"

#include "../e/exceptions.e"

extern cpu_t TODStarted;

cpu_t current;

HIDDEN void sysCreate(state_PTR state);
HIDDEN void sysTerminate();
HIDDEN void sysVerhogen(state_PTR old);
HIDDEN void sysPasseren(state_PTR old);
HIDDEN void sysSpecifyException(state_PTR caller);
HIDDEN void sysCPUTime(state_PTR state);
HIDDEN void sysWaitClock(state_PTR old);
HIDDEN void sysWaitIO(state_PTR old);
HIDDEN void pullUpAndDie(int type);

void copyState(state_PTR new, state_PTR old);


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
    pullUpAndDie(PGMTRAP);
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
        insertProcQ(&readyQueue, newProc);
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
    debugC(1);
    pcb_PTR new = NULL;
    debugC(3);
    int* semAdd = (int*) old -> s_a1;
    debugC(5);
    (*semAdd)++;
    debugC(7);
    if((*semAdd) <= 0)
    {
        debugC(9);
        new = removeBlocked(semAdd);
        debugC(11);
        if(!emptyProcQ(new))
        {
            debugC(13);
            insertProcQ(&readyQueue, new);
            debugC(4);
        }
    } 
    debugC(42);
}

void sysPasseren(state_PTR old)
{
    debugC(2);
    int* semAdd = (int *)old -> s_a1;
    debugC(48);
    (*semAdd)--;
    debugC(64);
    if((*semAdd) < 0)
    {
        debugC(80);
        copyState(old, &(currentProcess -> pcb_s));
        debugC(96);
        insertBlocked(semAdd, currentProcess);
        debugC(112);
        scheduler();
    }
    debugC(25);
}

void sysSpecifyException(state_PTR caller)
{
    int type = caller -> s_a1;
    state_PTR old = (state_PTR) caller -> s_a2;
    state_PTR new = (state_PTR) caller -> s_a3;

    switch(type)
    {
        case TLBTRAP:
            if(currentProcess -> oldTLB != NULL)
            {
                sysTerminate();
            }
            currentProcess -> newTLB = (state_PTR) new;
            currentProcess -> oldTLB = (state_PTR) old;
            break;
        case PROGTRAP:
            if(currentProcess -> oldPGM != NULL)
            {
                sysTerminate();
            }
            currentProcess -> newPGM = (state_PTR) new;
            currentProcess -> oldPGM = (state_PTR) old;
            break;
        case SYSTRAP:
            if(currentProcess -> oldSys != NULL)
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

    currentProcess -> pcb_time = currentProcess -> pcb_time + (t - TODStarted);
    state -> s_v0 = currentProcess -> pcb_time;
    STCK(TODStarted);
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
    int *semAdd;
    int interruptLine, deviceNum, isRead, index;
    interruptLine = old -> s_a1;
    deviceNum = old -> s_a2;
    isRead = old -> s_a3;
    debugC(4097);

    if(interruptLine < DISKINT || interruptLine > TERMINT)
    {
        debugC(4107);
        sysTerminate();
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
    switch(type)
    {
        case TLBTRAP:
            if(currentProcess -> oldTLB != NULL)
            {
                debugH(2);
                copyState((state_PTR) TBLMGMTOLDAREA, currentProcess -> oldTLB);
                copyState(currentProcess -> newTLB, &(currentProcess -> pcb_s));
                LDST(&(currentProcess -> pcb_s));
            }
            else
            {
                debugH(5);
                sysTerminate();
                scheduler();
            }
            break;
        case PROGTRAP: 
            if(currentProcess -> oldPGM != NULL)    
            {
                debugH(3);
                copyState((state_PTR) PGMTRAPOLDAREA, currentProcess -> oldPGM);
                copyState(currentProcess -> newPGM, &(currentProcess -> pcb_s));
                LDST(&(currentProcess -> pcb_s));
            }
            else
            {
                debugH(5);
                sysTerminate();
                scheduler();
            }
            break;
        case SYSTRAP: 
            if(currentProcess -> oldSys != NULL)
            {     
                debugH(4);    
                copyState((state_PTR) SYSCALLOLDAREA, currentProcess -> oldSys);
                copyState(currentProcess -> newSys, &(currentProcess -> pcb_s));
                LDST(&(currentProcess -> pcb_s));
            }
            else
            {
                debugH(5);
                sysTerminate();
                scheduler();
            }          
            break;        
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

