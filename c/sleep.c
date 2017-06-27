/* sleep.c : sleep device  */

#include <xeroskernel.h>
#include <xeroslib.h>

/* Your code goes here */
extern struct pcb* sleep_queue;
extern struct pcb *ready_queue_head;
extern struct pcb *ready_queue_tail;
extern struct pcb* idle_proc; 

/* Purpose: put p on sleep queue, but at the right place
			using a structure like delta list in number of ticks
 * Input: p a living PCB
 * Output: In future A3 it will output a number, but for now
 *			we assume it is 0 and hard-code in dispatcher
 */
extern void sleep(struct pcb* p){

	struct pcb * tmp;
	
	/* cannot sleep idle process */
	if (p->state == IDLE){
		return;
	}

	/* de-queue from ready queue if it comes from ready queue */
	ready_queue_head = reconsQ(ready_queue_head, p);
	if (ready_queue_head == NULL){
		/* add idle proc on readyQ */
		ready_queue_head = idle_proc;
		ready_queue_head->next = NULL;
		ready_queue_tail = idle_proc;
		ready_queue_tail->next = NULL;
	}
	
	/* when the sleep queue is empty */
	if (sleep_queue == NULL){
		sleep_queue = p;
	}
	/* sleep queue is not empty */
	else {
		/* the p has less tick than the first item */
		if (p->tick < sleep_queue->tick){
			sleep_queue->tick = sleep_queue->tick - p->tick;
			p->next = sleep_queue;
			sleep_queue = p;
		}
		/* the p->tick has a previous item, also works add end of queue */
		else {
			tmp = findPosition(sleep_queue, p->tick);
			p->next = tmp->next;
			tmp->next = p;
			p->tick = p->tick - prev_sum(tmp);
			if(p->next != NULL){
				p->next->tick = p->next->tick - p->tick;
			}
		}
	}
}

/* Purpose: tick by 1 tick for the first item on sleep queue 
 * Input: None
 * Output: None 
 */
extern void tick_function(void){
	/* nothing on sleepQ, no update */
	if (sleep_queue == NULL){
		return;
	}
	/* update time slice */
	sleep_queue->tick --;
	
	struct pcb* tmp = sleep_queue;
	struct pcb* tmpNext;
	/* wake up all that need to be wake up*/
	while (tmp != NULL && tmp->tick == 0){
		/* put this on the end of readyQ */
		tmpNext = tmp->next;
		//kprintf("wakeup pid = %d; ",tmp->PID );
		ready(tmp);			
		tmp = tmpNext;
	}
	/* adjust the sleep_queue head */
	sleep_queue = tmp;
}

/* Purpose: find the right position to insert p after 
   Assumption: cannot return NULL actually since we did the if check
   Input: head: the head of some queue
		  num: the number of ticks that a process originally have 
		  (before relative)
   Ouput: the new head of some queue 
 */
struct pcb * findPosition(struct pcb* head, unsigned int num){
	struct pcb * tmp = head;
	struct pcb * prev = NULL;
	unsigned int sum = 0;

	while (tmp != NULL){
		sum = sum + tmp->tick;
		if (sum > num){
			/*could be NULL, but that means insert before head */
			return prev;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	return prev;
}

/* Purpose: find the sum of all the ticks from head of sleep queue
			until curr
   Input: curr: current pcb
   Output: sum of all the ticks until curr
 */
extern unsigned int prev_sum(struct pcb *curr){
	unsigned int sum = 0;
	struct pcb* tmp = sleep_queue;
	while(tmp != curr){
		sum += tmp->tick;
		tmp = tmp->next;
	}
	sum = sum + curr->tick;
	return sum;
}

/* PURPOSE: take out p from the sleep queue and re-adjust the ticks
			for the rest of the queue 
   INPUT: p : the process to take out
*/
extern struct pcb * reconsSleepQ(struct pcb* p){
	struct pcb* tmp = sleep_queue;
	struct pcb* tmp_prev = NULL;
	while (tmp != NULL){
		if (tmp == p){
			if (tmp == sleep_queue){
				sleep_queue = sleep_queue->next;
			}
			if (tmp->next != NULL){
				tmp->next->tick += tmp->tick;
			}
			if (tmp_prev != NULL){
				tmp_prev->next = tmp->next;
			}
			tmp->next = NULL;
			break;
		}
		tmp_prev = tmp;
		tmp = tmp->next;
	}
	return sleep_queue;
}