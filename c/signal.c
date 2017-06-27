/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <i386.h>

extern struct pcb *ready_queue_head; /* head of ready queue */
extern struct pcb* sleep_queue;/* head of sleep queue*/
extern struct pcb* blockQ_head;/* block queue for reading from device*/
/* Your code goes here */

/*
 Purpose:	this function registers provided function handler for
			current process at the specifed signal entry.
 Input:		p - specifies the process to register handler to 
 			sig_id - signal number
 			newhandler - function pointer to register on signal table
 			oldHandler - pointer to old function pointer in the signal
 						table to save it.
 Output:	-1 - signal number is invalid
 			-2 - newhandler is in invalid address
 			0 - if handler is successfully installed
*/
extern int sighandler(struct pcb* p, int sig_id, void (*newhandler) (void*), void (**oldHandler) (void*)){
	/* invalid signal number*/
	if(!isValid(sig_id)){
		return -1;
	}
	/*invalid function address*/
	else if (!isValidAddress(newhandler)){
		return -2;
	}
	else {
		/* save old handler */
		*oldHandler = p->sig_table[sig_id];
		/* register new register*/
		p->sig_table[sig_id] = newhandler;
				
		return 0;
	}
}

/*
 Purpose:	checks if the provided signal number is valid
 Input:		sig_id - integer value specifies a signal number
 Output:	TURE - if the signal number is within 0 to 31
 			FALSE - otherwise
*/
extern int isValid(int sig_id){
	if (sig_id >= 0 && sig_id < 32){
		return TRUE;
	}
	return FALSE;
}

/*
 Purpose:	checks if the provided function is in valid memory space
 Input:		handler - function pointer to check
 Output:	TRUE - if function is not in HOLE
 			FALSE - otherwise
*/
int isValidAddress(void (*handler)(void*)){
	  // Check if address is in the hole
	if (((unsigned long) handler) >= HOLESTART && ((unsigned long) handler <= HOLEEND)) {
	  return FALSE;
	}
	else{
		return TRUE;
	}
	

}
/*
 Purpose:	this funtion causes the calling process to wait for process
 			specified by PID to terminate.
 Input: 	dest_pid - the pid of the process to wait
 			p - the process that does the wait
 Output:	0 - the call ternimates normally
 			-1 - process specifed by PID does not exist
 			-2 - current process is being signaled while waiting
*/
extern int p2pwait(int dest_pid,struct pcb* p){
	/* find process by pid*/
	struct pcb* dest_pcb = findprocess(dest_pid);
	if(dest_pcb == NULL){
		return -1;
	}
	/* we choose not to wait for a process that is blocked for waiting ,
		hw input, and sleeping since we don't want to waste cpu cycles*/
	else if(dest_pcb->state != READY){
		return -2;
	}
	else{	
		/* remove process from ready queue*/	
		reconsReadyQ(p);
		/* add process to waiting queue*/
		dest_pcb->block_q_wait = enqueue(dest_pcb->block_q_wait,p,BLOCK);
		p->waitingForPCB = dest_pcb;

		return 0;
	}

}

/*
 Purpose:	This process delivers the given signal to the specified process
 Input:		pid - specifies the process to send signal to 
 			sig_no - the signal number to send
 Output:	-1 - if PID is invalid
 			-2 - if signal number is invalid
 			0 - otherwise
*/
extern int signal(int pid, int sig_no){
	/* find process by pid*/
	struct pcb *target_pcb = findprocess(pid);
	if(target_pcb == NULL){
		return -1;
	}
	else if(!isValid(sig_no)){
		return -2;
	}else{/*process can signal to itself*/
		
		/* "enqueue" sig_no */
		target_pcb->sig_status[sig_no] = 1;

		/* if blocked for other signal, save result on pcb*/
		if(target_pcb->state == BLOCK){

			target_pcb->create_result = -2;
			
			/*dequeue from block queue, either from block_q_wait, or blockQ_head */
			if (target_pcb->waitingForPCB != NULL){
				target_pcb->waitingForPCB->block_q_wait = reconsQ(target_pcb->waitingForPCB->block_q_wait, target_pcb);
				target_pcb->waitingForPCB = NULL;
			}
			else {
				/* since we only have two cases, this must be blocked as waiting for keyboard inputs*/
				blockQ_head = reconsQ(blockQ_head, target_pcb);
			}
			/*enqueue to ready queue */
			ready(target_pcb);			
		}
		/* if target process is sleeping*/
		else if(target_pcb->state == SLEEP){
			/* set return value to sleep */
			target_pcb->create_result = prev_sum(target_pcb);
			/* remove from sleep queue*/
			sleep_queue = reconsSleepQ(target_pcb);
			ready(target_pcb);
		}
		return 0;
	}	
}

/*
 Purpose:	modifies the user process stack when there is signal to handle
 			so that the process jump to this function 
 Input:		handler -  user provided handler function
 			cntx -  old stack pointer value
 Output:
*/
extern void sigtramp(void (*handler)(void*), void* cntx){
	handler(cntx);
	syssigreturn(cntx);
}
