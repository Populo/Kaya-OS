#include "../h/types.h"
#include "../h/const.h"

/*
 * pcb.c
 * 
 * Contains functions for:
 * 	 Process Free List
 * 	   Allocate and free PCBs
 * 	 Process Block Queues Manager
 *     Make an empty Process queue
 *     Check if a Process queue is empty
 *     Insert PCBs to a Process queue
 *     See the next PCB in a Process queue
 * 	   Remove the first PCB from a Process queue
 *     Remove a specific PCB from it's Process queue
 *   Process Children Tree
 *     Insert children into a PCB
 *     Remove the first child from a PCB
 *     Remove a specific PCB from it's parent
 * 
 * Authors: Alex Fuerst, Aaron Pitman
 * 
 */
 
 /*
  * Pcb Free list is kept as a singly linked linear stack with the
  *  head as pcbList_h
  * 
  * Process Queue's are doubly linked circular queue with a tail pointer
  * 	the tails next is the queue's head
  * 
  * Child Tree is a double linked linear stack with a head pointer
  * 
  */


/**************************Global Definitions**************************/
/*The pointer to the head of the PCB Free List*/
HIDDEN pcb_PTR pcbList_h;

/*********************Free PCB List Implementation*********************/

/************************ PCB FREE LIST DEFINITIONS *******************/
/*  
    Insert the element pointed to by p onto the pcbFree list.
*/
void freePcb(pcb_PTR p){
	p -> pcb_next = pcbList_h;
	pcbList_h = p;
}

/* 
    Return NULL if the pcbFree list is empty. Otherwise, remove
    an element from the pcbFree list, provide initial values for 
    all fields.
*/
pcb_t *allocPcb(){
	pcb_PTR temp;
    if(pcbList_h == NULL){
        return NULL;
    }
	temp = pcbList_h;
	pcbList_h = pcbList_h -> pcb_next;
    
    /* set queue values to NULL */
    temp -> pcb_next = NULL;
    temp -> pcb_prev = NULL;
        
    /* children tree values to NULL */
    temp -> pcb_parent = NULL;
    temp -> pcb_child = NULL;
    temp -> pcb_nextSib = NULL;
    temp -> pcb_prevSib = NULL;

    return temp;
}


/* 
    Initialize the pcbFree list to contain all the elements
    if the static array or MAXPROC ProcBlk's. This method will
        be called only once during data structure initialization.
*/
void initPcbs(){
    static pcb_t procTable[MAXPROC];
    int i;
    pcbList_h = NULL;
    for(i=0;i < MAXPROC; i++){
        freePcb(&procTable[i]);
    }
}


/**********************************************************************/

/**************************** PROC QUEUE DEFINITIONS ******************/

/*  
    Initialize a variable to be a tail ptr to a proc queue
*/
pcb_PTR mkEmptyProcQ(){
	return NULL;
}

/*
    Return TRUE if queue whose tail is pointed to by tp is empty. 
    Return FALSE otherwise
*/
int emptyProcQ(pcb_PTR tp){
	return (tp == NULL);
}


/*  
    Insert ProcBlk pointed to by p into process queue 
    whose tail-pointer is pointed to by tp.
*/
void insertProcQ(pcb_PTR *tp, pcb_PTR p){
	
	/*If the queue is empty...*/
	if (emptyProcQ(*tp)){
		p->pcb_next = p;
		p->pcb_prev = p;
	} /*The queue is not empty*/
	else{
		/*Merge the new element into the list*/
		p->pcb_next = (*tp)->pcb_next;
		(*tp)->pcb_next->pcb_prev = p;
		(*tp)->pcb_next = p;
		p->pcb_prev = *tp;
	}
	
	/*Set the tail pointer to the new node*/
	*tp = p;
}


/*  
    Return a pointer to the first ProcBlk from the process queue whose
    tail pointer is pointed to by tp. Does not remove the ProcBlk. 
    Return NULL if the queue is empty.
*/
pcb_PTR headProcQ(pcb_PTR tp){
	return (tp -> pcb_next);
}


/*
    Remove the first (head) element from the process queue whose tail
    pointer is pointed to by tp. Return NULL if the queue is empty. 
    otherwise return pointer to the removed element. Update queue 
    as needed.
*/
pcb_PTR removeProcQ(pcb_PTR *tp){
	pcb_PTR ret;
	if(emptyProcQ(*tp)) { /* queue is empty */
		return NULL;
	} else if ((*tp) -> pcb_next == (*tp)) { /* only one item in queue */
		ret = (*tp);
		(*tp) = mkEmptyProcQ(); /* make tp an empty queue */
		return ret;
	} /* else */
	/* n items in queue */
	ret  = (*tp) -> pcb_next;
	(*tp) -> pcb_next -> pcb_next -> pcb_prev = (*tp);
	(*tp) -> pcb_next = ((*tp) -> pcb_next -> pcb_next);
	return ret;
}

/*  
    Remove ProcBlk pointed to by p from the process queue whose tail 
    pointer is pointed to by tp. Update the process queue's tail if 
    necessary. If ProcBlk is not in the queue, return NULL; otherwise
    return p.
*/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p) {
	pcb_PTR ret, temp;
	if(((*tp) == NULL) || (p == NULL)) {
		return NULL;
	}
	/* only one thing in queue and it is what we want */
	if((*tp) == p){
		
		if ((((*tp) -> pcb_next) == (*tp))) {
			ret = (*tp);
			(*tp) = mkEmptyProcQ();
			return ret;
		} else {
			(*tp)->pcb_prev->pcb_next = (*tp)->pcb_next;
			(*tp)->pcb_next->pcb_prev = (*tp)->pcb_prev;
			*tp = (*tp)->pcb_prev;
		}
		return p;
	} else {
	/* node is somewhere else, start at pcb_next */
	temp = (*tp) -> pcb_next;
	while(temp != (*tp)) {
		/* found node ? */
		if(temp == p){
			/* unleave node and return it */
			ret = temp;
			ret -> pcb_prev -> pcb_next = ret -> pcb_next;
			ret -> pcb_next -> pcb_prev = ret -> pcb_prev;
			ret -> pcb_next = NULL;
			ret -> pcb_prev = NULL;
			return ret;
		}
			temp = temp -> pcb_next;
		}
		/* node not in list here */
		return NULL;
	}
}

/********************** CHILD TREE DEFINITIONS ************************/
/*
	Return TRUE if pcb_PRT p has no children. Returns FALSE otherwise.
*/ 
int emptyChild(pcb_PTR p){
	return ((p -> pcb_child) == NULL);
}

/*
	Make pcb pointed to p by a child of prnt
	inserts to the front of the child list
*/
void insertChild(pcb_PTR prnt, pcb_PTR p){
	if(emptyChild(prnt)){
		/* no children */
		prnt -> pcb_child = p;
		p -> pcb_parent = prnt;
		p -> pcb_prevSib = NULL;
		p -> pcb_nextSib = NULL;
		return;
	} else {
		/* multiple children */
		prnt -> pcb_child -> pcb_prevSib = p;
		p -> pcb_nextSib = prnt -> pcb_child;
		p -> pcb_prevSib = NULL;
		prnt -> pcb_child = p;
		p -> pcb_parent = prnt;
		return;
	}
}

/*
	Make first child of p no longer a child of p.
	return NULL if p initially has no children
	otherwise return pointer to removed process
*/
pcb_PTR removeChild(pcb_PTR p){
	pcb_PTR ret; /* return value */
	if(emptyChild(p)){ /* p has no children */
		return NULL; /* you're bad and you should feel bad */
	}
	ret = p -> pcb_child;
	if(p -> pcb_child -> pcb_nextSib == NULL){
		/* only one child */
		p -> pcb_child = NULL;
		ret -> pcb_parent = NULL;
		ret -> pcb_nextSib = NULL;
		ret -> pcb_prevSib = NULL;
		return ret;
	} else {
		/* has siblings */
		p -> pcb_child = p -> pcb_child -> pcb_nextSib;
		p -> pcb_child -> pcb_prevSib = NULL;
		ret -> pcb_parent = NULL;
		ret -> pcb_nextSib = NULL;
		ret -> pcb_prevSib = NULL;
		return ret;		
	}
}

/*
	Make pcb pointed to by p no longer a child.
	return NULL if p has no parent, otherwise return p
	p can be in any point of the child list 
*/
pcb_PTR outChild(pcb_PTR p){
	if((p == NULL) || (p -> pcb_parent == NULL)){
		/* p is not a child */
		return NULL;
	}
	if((p -> pcb_parent -> pcb_child) == p){
		/* am newest child */
		return removeChild(p -> pcb_parent);
	}
	if ((p -> pcb_nextSib) == NULL){
		/* p is at the end of child list */
		p -> pcb_prevSib -> pcb_nextSib = NULL;
		p -> pcb_parent = NULL;
		return p;
	}
	if (((p -> pcb_prevSib) != NULL) && ((p -> pcb_nextSib) != NULL)){ 
		/* p is is a middle child */
		p -> pcb_nextSib -> pcb_prevSib = p -> pcb_prevSib;
		p -> pcb_prevSib -> pcb_nextSib = p -> pcb_nextSib;
		p -> pcb_parent = NULL;
		return p;
	}
	/* should never get to this */
	return NULL;
}

/********************* END CHILD TREE DEFINITIONS *********************/

