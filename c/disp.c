/* disp.c : dispatcher
 */

#include <icu.h>
#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <kbd.h>

/* Manage ready queue */
extern struct pcb *ready_queue_head; /* head of ready queue */
extern struct pcb *ready_queue_tail; /* tail of ready queue */
extern struct pcb *stopped_queue; /* head of stopped queue */
extern struct pcb* sleep_queue;  /* head of stopped queue */
/* for searching for process */
extern struct pcb pcb_table[PCB_TABLE_SIZE];
/* pointer to the idle process for easy access */
extern struct pcb* idle_proc; 

struct pcb *process;

/* Purpose: Managing processes by looking at what each one wanted 
 *			to do and then executing the right actions associated
 *			Each interrupted user process will come back to dispatcher
 *			After processing the request, dispatcher will call contextswitch
 *			to go back to an user process
 * Input: none
 * Output: none
 */
extern void dispatch(void){

	int create_result;
    fancyfuncptr     fp;
    funcptr		real_fp;
    int         stack;
    int 		pid;
    va_list     ap;
    char * 		str;
    unsigned long num;
    int* pid_pointer;
    unsigned long* num_pointer;
	int sig_id;
	unsigned int tick_num;
    void (*new_handler) (void*) ;
    void (**old_handler) (void*) ;
    unsigned long* esp_old;
    processStatuses * ps;
    int device_no;
    int fd;
    void* buff;
    int bufflen;
	unsigned long *command_addr;
	
	process = next();

	for(;;){

		/* calls contextswitch() to manipulate the transition between user
		 * and kernel */
		int request = contextswitch(process);
	
		switch(request){
			case(CREATE) : 
				/* process->func_start and process->stack_size are modified in 
				 * create.c or in ctsw.c */
				ap = (va_list)process->args;
    			fp = (fancyfuncptr)(va_arg( ap, int ) );
    			real_fp = (funcptr)(va_arg( ap, int));
    			stack = va_arg( ap, int );
				create_result = create(fp,real_fp, stack);
				process->create_result = create_result;
				break;
			case(YIELD) : 
				/* yield: put the current process on the tail of ready queue and 
				 *get the head of ready queue to be the next process */
				ready(process); 
				process = next();	
				break;
			case(STOP) : 
				/* stop: free up space for the current process in both PCB table
				 * and its associated stack, get the next ready process */
				cleanup(process); 
				process = next(); 
				break;
			case(GETPID) :
				/* get PID */
				process->create_result = process->PID;
				break;
			case(PUTS) :
				/* prints str */			
				ap = (va_list)process->args;
				str = va_arg( ap, char * );				
				kprintf("%s",str);
				break;
			case(KILL) :
				/* mark a signal # for a destination process */			
				ap = (va_list)process->args;
				pid = va_arg( ap, int );
        		sig_id = va_arg(ap, int);
        		process->create_result = kill(process,pid,sig_id);
				break;
			case(SEND) :
				/* send a message */
				ap = (va_list)process->args;
				pid = va_arg( ap, int );
				num = va_arg( ap, unsigned long );
				process->create_result = send(pid, process, num);
				process = next();
				break;
			case(RECV) :
				ap = (va_list)process->args;
				pid_pointer = va_arg( ap, int * );
				num_pointer = va_arg( ap, unsigned long * );
				process->create_result = receive(pid_pointer,process,num_pointer);
				process = next();
				break;
			case(TIMER_INT) :
				/* timer interrupt */			
				tick_function();
        		process->cpu_time++;
				ready(process);
				process = next();
				end_of_intr();	
				break;
			case(KEYBOARD_INT) :
				/* keyboard interrupt */
				kbd_int_handler();
				end_of_intr();
				break;	
			case(SLEEP) :
				ap = (va_list)process->args;
				tick_num = va_arg( ap, unsigned int );
				/* suggested conversion value is 10 */				
				tick_num = tick_num / 10 + ((tick_num%10)?1:0);
				/*the slice may a little less than requested */
				process->tick = tick_num;
				sleep(process);
				process->create_result = 0;
				process = next();				
				break;
	      	case(CPUTIME) :
	      		/* get CPU time for all running processes */
	        	ap = (va_list)process->args;
	        	ps = va_arg( ap, processStatuses * );
	        	process->create_result = getCPUtimes(process,ps);
	        	break;
	      	case(SETSIGNAL) :
				/* set this process's signal handler for a particular sig_id */
	        	ap = (va_list)process->args;
	        	sig_id = (va_arg(ap, int));
	        	new_handler = (va_arg( ap, void (*) (void*)));
	        	old_handler = (va_arg( ap, void (**) (void*)));
	       	 	process->create_result = sighandler(process,sig_id,new_handler,old_handler);
	        	break;
	      	case(SIGRETURN) :
	      		/* used in pair with KILL 
	      		to reset stack as if nothing happened */
	        	ap = (va_list)process->args;
	        	esp_old = va_arg( ap, unsigned long* );
	        	process->esp = esp_old;
	        	break;
	      	case(WAIT) :
				/* wait for destination process to die first */      	
	        	ap = (va_list)process->args;
	        	pid = va_arg( ap, int );
	        	process->create_result = p2pwait(pid,process);
	        	break;  
	        case(OPEN) :
	        	/* open a device */
	        	ap = (va_list)process->args;
	        	device_no = va_arg(ap, int);
	        	process->create_result = di_open(process, device_no);
	        	break;
	        case(CLOSE) :
	        	/* close a device */
	        	ap = (va_list)process->args;
	        	device_no = va_arg(ap, int);
	        	process->create_result = di_close(process, device_no);	        
	        	break;
	        case(WRITE) :
	        	/* write to a device based on fd # */
	        	ap = (va_list)process->args;
	        	fd = va_arg(ap, int);
	        	buff = (void *)va_arg(ap, int);
	        	bufflen = va_arg(ap,int);
	        	process->create_result = di_write(process,fd,buff,bufflen);					        
	        	break;
	        case(READ) :
	        	/* read from a device based on fd # */
	        	ap = (va_list)process->args;
	        	fd = va_arg(ap, int);
	        	buff = (void *)va_arg(ap, int);
	        	bufflen = va_arg(ap,int);        	
	        	process->create_result = di_read(process,fd,buff,bufflen,0);	        
	        	break;
	        case(IOCTL) :
	        	/* command a particular command to a device
	        		the reaction is device specific */
	        	ap = (va_list)process->args;
	        	fd = va_arg(ap, int);
	        	command_addr = (unsigned long *)va_arg(ap,int);
	        	process->create_result = di_ioctl(process, fd, command_addr);
	        	break;      
			}
		/*the current process might be blocked, in that case go to next */
		if (process->state == BLOCK){
			process = next();
		}
	}
}

/* Purpose: Get the next process in the ready queue 
 * Input: none
 * Output: a pointer to the next process 
 */
struct pcb* next(void){
	struct pcb* tmp = ready_queue_head;
		
	/* if there is only one process */
	if (ready_queue_head->next == NULL){
		return ready_queue_head;
	}
	/* else remove the first one on the ready queue and return it */
	else {
		ready_queue_head = ready_queue_head->next;
	}
	if (tmp != idle_proc){
		tmp->state = RUNNING;
	}
	tmp->next = NULL;
	return tmp;
}

/* Purpose: put a process on the end of ready queue
 * Input: process: the process to be put on the end of ready queue
 * Output: none 
 */
extern void ready(struct pcb *process){
	/* special case: if there is only 1 process */
	if (ready_queue_head == ready_queue_tail){
		if (process ==ready_queue_head){
			/* do nothing */
			return;
		}
		/* first process is idle proc, remove it and add process */
		else if (ready_queue_head == idle_proc){
			ready_queue_head = process;
			ready_queue_tail = process;
			process->next = NULL;
			process->state = READY;
			idle_proc->next = NULL;
		}
		/* first process is not idle, second process is idle */
		else if (process == idle_proc){
			return;
		}
		/* first process is not idle proc, just add the other process */
		else {
			ready_queue_head->next = process;
			ready_queue_tail = process;
			process->next = NULL;
			process->state = READY;
		}
	}
	else if (process != ready_queue_head){ /* do not need to move head */
		ready_queue_tail->next = process;
		process-> next = NULL;
		process->state = READY;
		ready_queue_tail = ready_queue_tail->next;
		return;
	}
	else { /* need to move head from READY to READY */
		ready_queue_head = ready_queue_head->next;
		ready_queue_tail->next = process;
		process->state = READY;
		ready_queue_tail = ready_queue_tail->next;
		process->next = NULL;
	}
}

/* Purpose: put a process on the head of stopped queue 
 * Input: process: this is the process to be freed from both PCB table and
 *        free the allocated process stack
 * Output: none
 */
void cleanup(struct pcb *process){
	if (process == NULL){
		/*don't clean up nothing */
		return;
	}
	if( process->state == IDLE){
		/*for real, don't try to clean up idle my friend */
		return;
	}
	/* before terminating process, let everyone who is waiting for it know
	 * to avoid vampire cases */
	memorial_service(process);
	/* if the process is the head of ready queue */
	if (process == ready_queue_head){
		/* if there is only 1 process */
		if (ready_queue_head == ready_queue_tail){
			ready_queue_head = idle_proc;
			ready_queue_tail = idle_proc;
		}
		else {
			/* move down one head of ready queue */
			ready_queue_head = ready_queue_head->next;
		}
		/* deallocate space */ 
		process -> next = stopped_queue; /*it actually acts like a stack */
		process->state = STOP;
		stopped_queue = process;
		/* free the allocated memory stack */
		kfree(process->kmem_space);
	}
	else { /* not the head of ready queue, head does not need to move */
		/* this means this process was just running */
		process -> next = stopped_queue; /*it actually acts like a stack */
		process->state = STOP;
		stopped_queue = process;
		kfree(process->kmem_space);
	}
}

/* Purpose: printing queue for debug 
 * 
 */
extern void print_queue(struct pcb *p){
	while(p != NULL){
		/* going to use a simplified version to save space */
		kprintf(" %d, ", p->PID);
		p = p->next;
		if (p == NULL){
			kprintf("; ");
		}
	}
}

/* 	Purpose: find the process by a PID
	Input: pid: the number that uniquely identifies a process 
	Output: return null if cannot find 
			returns the pcb pointer if it finds one
*/
extern struct pcb* findprocess(int pid){
	struct pcb * tmp;
	for (int i=0; i<PCB_TABLE_SIZE; i++){
		tmp = &pcb_table[i];
		if (tmp->PID == pid && tmp->state != STOP){
    //if(tmp->PID == pid){
			return tmp;
		}
	}
	return NULL;
}

/* 	Purpose: Let all processes that are waiting for p know that p is dead
			So they can go on with other instructions while knowing that
			send/receive as a -1 mistake 
	Assumption: p exists 
	Input: p: a pointer to the pcb p 
	Output: None
*/
void memorial_service(struct pcb* p){
	/* go through both sender and receiver queue */
	/* notify everyone on the queue who is blocked that you are free to go */
	/* since p is dead */
	struct pcb * tmp;
	struct pcb * tmp_head;
	/* for sender */
	tmp = p->block_q_sender;
	while (tmp != NULL){
		tmp_head = tmp;
		tmp->create_result = -1;
		tmp = tmp->next;
		if (tmp_head->state != STOP){
			ready(tmp_head);
		}
	}
	/* for receiver */
	tmp = p->block_q_receiver;
	while (tmp != NULL){
		tmp_head = tmp;
		tmp->create_result = -1;
		tmp = tmp->next;
		if (tmp_head->state != STOP){
			ready(tmp_head);
		}
	}
  	/* for waiting */
  	tmp = p->block_q_wait;
  	while (tmp != NULL){
    	tmp_head = tmp;
    	tmp->create_result = 0;
    	tmp->waitingForPCB = NULL;
    	tmp = tmp->next;
    	if (tmp_head->state != STOP){
      		ready(tmp_head);
    	}
  	}
}

/* 	Purpose: a helper function for debugging about pcb states
			by printing the first 10 entries in PCB table 
	Input: none
	Output: none 
*/
extern void print_pcb_state(void ){
	int i;
	for (i=0; i<10; i++){
		kprintf(" p[%d].state = %d; ", i, pcb_table[i].state);
	}
	kprintf("\n");
}

/* 	PURPOSE: get cpu time for a specific pid
	INPUT: pid that identifies the process
	OUTPUT: cpu_time in milliseconds
	NOTE: cannot get cpu time for a stopped process
		***deprecated***
*/
int getcputime(int pid){
  if(pid == -1){
    /*return current process cpu time*/
    return process->cpu_time;
  }
  else if(pid == 0){
    /*return idle proc cpu time*/
    return idle_proc->cpu_time;
  }
  else{
    struct pcb* p = findprocess(pid); 
    /*process with given pid is not found*/
    if(p == NULL){
      return -1;
    }  
    return p->cpu_time; 
  }

}


extern char * maxaddr;
/* 	PURPOSE: getCPUtimes from all alive proccesses
	INPUT: 	p - a pointer into the pcbtab of the currently active process
			ps  - a pointer to a processStatuses structure that is 
	OUTPUT: number of alive processes
	NOTE: this code comes from teacher 
*/ 
int getCPUtimes(struct pcb *p, processStatuses *ps) {
  
  int i, currentSlot;
  currentSlot = -1;

  // Check if address is in the hole
  if (((unsigned long) ps) >= HOLESTART && ((unsigned long) ps <= HOLEEND)) 
    return -1;

  //Check if address of the data structure is beyone the end of main memory
  if ((((char * ) ps) + sizeof(processStatuses)) > maxaddr)  
    return -2;

  // There are probably other address checks that can be done, but this is OK for now


  for (i=0; i < PCB_TABLE_SIZE; i++) {
    if (pcb_table[i].state != STOP) {
      // fill in the table entry
      currentSlot++;
      ps->pid[currentSlot] = pcb_table[i].PID;
      ps->status[currentSlot] = p->PID == pcb_table[i].PID ? RUNNING: pcb_table[i].state;
      ps->cpuTime[currentSlot] = pcb_table[i].cpu_time * MILLISECONDS_TICK;
    }
  }

  return currentSlot;
}

/* 	PURPOSE: marks signal for a process
	INPUT: 	currP - current process
			pid - pid of the process to be signaled
			signal_number - the signal # to be signaled
	OUTPUT: -712 - cannot find destination process
			-651 - invalid signal #
			-2 - cannot signal self
			-1 - cannot signal idle process
*/
int kill(struct pcb* currP, int pid , int signal_number){
  struct pcb* p_to_kill = findprocess(pid);
  int result;
  if (p_to_kill == NULL){
    result = -712;
  }
  else if(!isValid(signal_number)){
    result = -651;
  }
  else if (currP->PID == pid){
    result = -2;
  }
  /* we knew that 0 is the PID of idle by design choice */
  else if(pid == 0){
    result = -1;
  }
  else{
    result = signal(pid,signal_number);
  }
  return result;
}