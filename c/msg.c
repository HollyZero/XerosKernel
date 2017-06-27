/* msg.c : messaging system 
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>

/* Your code goes here.... */


extern struct pcb * receiveFromAnyQ;
extern struct pcb * ready_queue_head;
extern struct pcb *ready_queue_tail;
extern struct pcb* idle_proc; 
struct pcb * empty_pointer = NULL;
extern struct pcb pcb_table[PCB_TABLE_SIZE];

/* Purpose: send message (a number) from srcPCB to the process 
			identified by destPID; buffer is in the PCB of receiver
			if the destPCB is not receiving from 0 or this srcPCB yet,
			block srcPCB, else pass the message and put both processes
			back on ready queue
 * Input: destPID: an integer identifying receiver
 		  srcPCB: a PCB identifying sender
 		  num: the message being sent
 * Output: 	0 if the message is sent, or if sender is being blocked
 			-1 if the receiver does not exist
 			-2 if the receiver and sender are the same processes
 			-3 any other error
 */
extern int send(int destPID, struct pcb* srcPCB, unsigned long num){

	int result; 
	/*find PCB slot using destPID*/
	struct pcb *dest_pcb = findprocess(destPID);
	/* if sender PID does not exist,return error code -3 */
	if (findprocess(srcPCB->PID) == NULL){
		result = -3;
	}
	/* receiver PID is valid */
	else if (dest_pcb != NULL){
		/* if it wants to send to itself, error code -2*/
		if(destPID == srcPCB->PID){
			result = -2;
		}
		/* CASE: receiver did not call recv() yet */	
		else if(dest_pcb->state != BLOCK && dest_pcb->state != RUNNING){

			/* remove sender from ready queue */
			reconsReadyQ(srcPCB);

			/* save message on PCB */
			srcPCB->s_num = num; 

			/* wait for receiver while being blocked */
			dest_pcb->block_q_sender = enqueue(dest_pcb->block_q_sender , srcPCB, BLOCK);
			result = 0;
		}
		/* CASE: receiver state is blocked */
		else if(dest_pcb->state == BLOCK){
			/* CASE: if receiver blocked because of recv(sender) */
			if(isInQ(srcPCB->block_q_receiver, dest_pcb)){

				/*save message in receiver buffer on PCB*/
				dest_pcb->buf = num;	
				*(dest_pcb->u_p_num) = num;	

				/*take receiver out of blocked queue */		
				srcPCB->block_q_receiver = reconsQ(srcPCB->block_q_receiver, dest_pcb);
				
				/*put both sender and receiver on ready queue*/
				ready(dest_pcb);
				ready(srcPCB);
				result = 0;
			}
			/* CASE: if receiver called recv(0) */
			else if (isInQ(receiveFromAnyQ, dest_pcb)){

				/* save message on receiver buffer*/
				dest_pcb->buf = num;
				*(dest_pcb->u_p_num) = num;

				/*take receiver out of receive-any-queue */
				receiveFromAnyQ = reconsQ(receiveFromAnyQ, dest_pcb);

				/*put both processes on ready queue*/
				ready(dest_pcb);
				ready(srcPCB);
				result = 0;				
			}
			/* CASE: receiver blocked for some other processes */
			else {
				/* PS. this could result in a deadlock*/
				/* save message on sender buffer*/
				srcPCB->s_num = num; 
				
				/* add sender on receiver's blocked queue */								
				dest_pcb->block_q_sender = enqueue(dest_pcb->block_q_sender , srcPCB, BLOCK);
				result = 0;				
			}
		}
		else { 
			result = -3;
		}
	}
	else {
		/* if destination process does not exist or terminated*/
		result = -1;
	}
	return result;
}   

/* PURPOSE: Receive message from a sender process, if sender has not sent
			yet, go on sender's block-queue, if receiving from 0, put on
			receive-any-queue. If sender has already sent, put both process
			on ready queue.
 * Input:	srcPID: sender's PID
 			destPCB: receiver (myself)
 			len: pointer to memory allocated by user to store received message
 * Output: 	0 if the message is received, or if receiver is being blocked
 			-1 if the sender does not exist
 			-2 if the receiver and sender are the same processes
 			-3 any other error
 */
extern int receive(int *srcPID, struct pcb* destPCB, unsigned long* len){
	
	int result;
	/* use srcPID to find sender PCB location*/
	struct pcb *srcPCB = findprocess(*srcPID);
	struct pcb *tmp_h;  

	/* check parameters, if invalid,return -3 */
	if (destPCB == NULL){
		return -3;
	}
	if (findprocess(destPCB->PID) == NULL){
		return -3;
	}

	/* CASE: if receiver is receiving from any sender*/
	if (*srcPID == 0){
		
		/* if blocked queue is not empty */
		if (destPCB->block_q_sender != NULL){
			tmp_h = destPCB->block_q_sender;
			destPCB->buf = tmp_h->s_num;
			/* take out the first sender on queue from the queue */
			destPCB->block_q_sender = reconsQ(destPCB->block_q_sender, destPCB->block_q_sender);
			/* put both process on ready queue */
			ready(tmp_h);
			ready(destPCB);
			/* save the message to len */
			*len = destPCB->buf;			
			result = 0;
		}
		else{ /*if blocked queue is empty, put receiver on receive-any-Q */
			reconsReadyQ(destPCB);
			receiveFromAnyQ = enqueue(receiveFromAnyQ, destPCB, BLOCK);
			result = 0;
		}
	}
	/* CASE: sender is invalid, return -1 */
	else if (srcPCB == NULL){
		result = -1; 
	}
	/* CASE: receiving from self, return -2 */
	else if (*srcPID == destPCB->PID){
		result = -2;
	}
	/* CASE: sender has not sent yet */
	else if (srcPCB->state != BLOCK){
		//kprintf(" |receiver: no sender yet| ");	
		kprintf("                           ");
		/*remove receiver from ready queue */
		reconsReadyQ(destPCB);
		kprintf("                           ");
		kprintf("                           ");
		/* save message on PCB*/
		destPCB->u_p_num = len;
		/*add receiver on sender's blocked queue */	
		srcPCB->block_q_receiver = enqueue(srcPCB->block_q_receiver, destPCB, BLOCK);
		result = 0;
	}
	/* CASE: if sender's state is blocked */
	else if (srcPCB->state == BLOCK){

		/*CASE: if sender is on receiver's blocked queue */
		if (isInQ(destPCB->block_q_sender, srcPCB)){
			/* both are ready to exchange message */
			/* save message on PCB*/
			destPCB->buf = srcPCB->s_num;
			*len = destPCB->buf;
			/* remove sender from receiver's blocked queue */
			destPCB->block_q_sender = reconsQ(destPCB->block_q_sender, srcPCB);
			/* put both process on ready queue */
			ready(srcPCB);
			ready(destPCB);
			result = 0;
		}
		else {/* PS. might cause deadlock*/
			/* CASE sender is blocked for some other process */			
			srcPCB->block_q_receiver = enqueue(srcPCB->block_q_receiver, destPCB, BLOCK);
			result = 0;
		}
	}

	return result;

} 

/* Purpose: add the current pcb to the end of the queue given, with the
			desired state
   Input:	head - head of the provided queue
   			curr - PCB to add to the end of the queue
   			state - change the state of curr after adding it on the queue
   Output:	PCB pointer of the head of the queue
*/
extern struct pcb * enqueue(struct pcb* head, struct pcb* curr, int state){
	struct pcb* tmp = head;
	if(tmp == NULL){
		head = curr;
		curr->next = NULL;
		curr->state = state;
	}
	else{
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = curr;
		curr->next = NULL;
		curr->state = state;
	}
	return head;
}

/* Purpose: Checks if the given pcb block is in the provided queue
   Input:	head - specifies the head of the provided queue
   			curr - the process to check wether it's in queue
   Output: 0 - the process is not in queue
   		   1 - the process is in queue
*/

int isInQ(struct pcb* head, struct pcb* curr){
	struct pcb * tmp = head;
	while (tmp != NULL){
		if (tmp == curr){
			return TRUE;
		}
		tmp = tmp->next;
	}
	return FALSE;
}

/* PRECONDITION: curr exists in the queue already */
/* Purpose: remove the current process from queue and reconstruct 
			the queue.
   Input:	head - specifies the head of the provided queue
   			curr - the PCB to remove from the queue
   Output:	return the head of the resulting queue
*/
extern struct pcb * reconsQ(struct pcb* head, struct pcb* curr){
	/* no queue at all */
	if (head == NULL){
		/*do nothing */
	}
	/* first item need to be dequeued */
	else if (head == curr){
//		kprintf("im IN HEREE");
		if (head->next == NULL){
			head = NULL;
		}
		else {
			head = head->next;
		}
		curr->next = NULL;		
	}
	else {
		/* some item in the middle need to be dequeued*/
		struct pcb * tmp = head->next;
		struct pcb * tmp_prev = head;	
		while (tmp != NULL){
			if (tmp == curr){
				tmp_prev->next = tmp->next;
				tmp->next = NULL;
				return head;
			}
			tmp_prev = tmp;
			tmp = tmp->next;
		}
	}
	return head;
}

/* Purpose: remove the specified pcb from the ready queue
   Input:	curr - the pcb to remove from the ready queue
   Output:	
*/
extern void reconsReadyQ(struct pcb* curr){
	//struct pcb * head = ready_queue_head;

	/* must have a queue with at least 1 item*/
	/* first item need to be dequeued */
	if (ready_queue_head == curr){
		if (ready_queue_head->next == NULL){
			/* only 1 item in the queue, de-queue this
				and make idle process on this queue */
			ready_queue_head = idle_proc;
			ready_queue_tail = idle_proc;
			ready_queue_head->next = NULL;
			ready_queue_tail->next = NULL;
		}
		else {
			ready_queue_head = ready_queue_head->next;
		}
	}
	else {
		/* some item in the middle need to be dequeued*/
		struct pcb * tmp = ready_queue_head->next;
		struct pcb * tmp_prev = ready_queue_head;	
		while (tmp != NULL){
			if (tmp == curr){
				if (tmp->next == NULL){
					ready_queue_tail = tmp_prev;
					ready_queue_tail->next = NULL;
				}
				tmp_prev->next = tmp->next;
				tmp->next = NULL;
				return;
			}
			tmp_prev = tmp;
			tmp = tmp->next;
		}
	}
}
