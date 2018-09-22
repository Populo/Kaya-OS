#include "../h/types.h"
#include "../h/const.h"

#include "../e/exceptions.e"

void sysCallHandler()
{
    state_PTR old;
    int sysCall;

    old  = (state_PTR) SYSCALLOLDAREA;
    
    old -> s_status; /* user mode or kernel mode */
    sysCall = old -> s_a0; /* which syscall was executed */

    switch(sysCall)
    {
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            break;
        case 8:
            break;
        default: /* handle 9-255 */
            break;
    }
}