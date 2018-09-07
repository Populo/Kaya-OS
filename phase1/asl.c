#include "../h/types.h"
#include "../h/const.h"

#include "../e/asl.e"

HIDDEN void allocSemd()
{

}

HIDDEN void freeSemd() 
{

}

/*
 * searches the ASL for the provided node and returns the 
 * node PREVIOUS to the found (or not) node
 * ex: 10->20->30
 * searching for 30 returns 20
 * searching for 25 returns 20
 */
HIDDEN void searchASL()
{

}

/*
 * searches semdlist for semAdd
 * 2 cases
 *   Found -> insertProcQ(semAdd->tp, p)
 *   Not Found -> 
 *        new semd_t with empty tp
 *        add new semd_t into ASL (Sorted by s_semAdd)
 *        same process as found
 */
int insertBlocked (int *semAdd, pcb_PTR p)
{
    return 0;
}

/*
 * search ASL for given semAdd
 * 2 cases:
 *   not found: error case (return null)?
 *   found: removeProcQ on s_procQ returns pcb_PTR
 *     2 cases:
 *          processQueue is not empty -> done
 *          processQueue is Empty ->
 *              deallocate semd node
 */
pcb_PTR removeBlocked (int *semAdd)
{
    return NULL;
}

/*
 * same as removeBlocked except call outProcQ instead of removeProcQ
 */
pcb_PTR outBlocked (pcb_PTR p)
{
    return NULL;
}

/*
 * same as remove/out except call headProcQ and do not deallocate
 */
pcb_PTR headBlocked (int *semAdd)
{
    return NULL;
}

/*
 * create ASL of length MAXPROC
 */
void initASL()
{

}