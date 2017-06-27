/* create.c : create a process*/

#include <xeroskernel.h>

extern struct pcb *ready_queue_head; /* head of ready queue */
extern struct pcb* idle_proc; 
extern struct pcb *ready_queue_tail; /* tail of ready queue */
extern struct pcb *stopped_queue; /* head of stopped queue */
extern int p_count; /* keep track of pid globally */
extern struct pcb pcb_table[PCB_TABLE_SIZE];

struct fake_param {
	unsigned long return_addr;
	unsigned long param;
};

/* Purpose: Create a new process 
 * Input: *func: the pointer to the first instruction of a function
 *		  stack: the size of the process stack
 * Output: 1 if created successfully, 0 if failed
 */
extern int create(void (*func) (void (*) (void)),void (*real_func) (void),int stack){
	
	/* if there is no free space, return 0 */
	if (stopped_queue == NULL){
		return -1;
	}
	/* can only have 1 idle process, no zuo no die */
	if (real_func == &idleproc && pcb_table[0].state == IDLE ){
		return -1;
	}
	
	/* find a free pcb from pcb table */
	struct pcb* tmp;
	if (real_func == &idleproc){
		tmp = &pcb_table[0];
	}
	else {
		tmp = stopped_queue;
		stopped_queue = stopped_queue->next;
	}
	
	/* allocate this pcb */
	tmp->PID = p_count; 
	p_count++;
	if (real_func == &idleproc){
		tmp->state = IDLE;
	}
	else {
		tmp->state = READY;
	}
	tmp->next = NULL;
	tmp->kmem_space = kmalloc(stack);
	/* move esp to the end of the memory space */
	tmp->esp = tmp->kmem_space + stack/sizeof(unsigned long);
	/* store start of function, result of this function, and the size of the stack in PCB */
	/* we could also use the context frame, but we are not for now */ 
	tmp->args = 0;	
	tmp->create_result = 0;
	tmp->block_q_sender = NULL;
	tmp->block_q_receiver = NULL;
	tmp-> buf = 0;
	tmp->s_num = 0;
	tmp->tick = 0;
	tmp->u_p_num = NULL;
    tmp->cpu_time = 0;
    tmp->u_buff = NULL;
    tmp->u_buff_len = 0;
    int i;
	  for(i = 0; i<SIG_TABLE_SIZE; i++){
	    tmp->sig_table[i] = NULL;
	    tmp->sig_status[i] = 0;
	  }
	  tmp->block_q_wait = NULL; 
	  tmp->waitingForPCB = NULL;


	/* initialize the parameter on the stack to imitate paramters */
	struct fake_param *fp = (struct fake_param *) (tmp->esp - sizeof(struct fake_param)/sizeof(unsigned long));
	fp->return_addr = 0;
	fp->param = (unsigned long) real_func;
	tmp->esp = (unsigned long *) fp;
	
	/* initialize context frame on stack */
	/* find the right position for start of context frame plus a safety margin */
	struct context_frame *ctfm = (struct context_frame *)(tmp->esp - sizeof(struct context_frame)/sizeof(unsigned long));
	ctfm->edi = 0;
	ctfm->esi = 0;
	ctfm->ebp = 0;
	ctfm->esp = 0;
	ctfm->ebx = 0;
	ctfm->edx = 0;
	ctfm->ecx = 0;
	ctfm->eax = 0;
	ctfm->iret_eip = (unsigned long )func;
	ctfm->iret_cs = getCS();
	ctfm->eflags = 0x00003200;
	/* update esp value */
	tmp->esp = (unsigned long *) ctfm;	

	/* special case: idleproc */
	if (ready_queue_head == idle_proc && ready_queue_tail == idle_proc){
		ready_queue_head = tmp;
		ready_queue_tail = tmp;
	}
	else {
		ready(tmp);
	}
	
	return tmp->PID;
}