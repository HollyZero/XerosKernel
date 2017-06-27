/* ctsw.c : context switcher
 */

#include <icu.h>
#include <i386.h>
#include <xeroskernel.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

static void *k_stack; /* never used because we do not yet need to get anything
					   * on the kernel stack */
static unsigned long *ESP;
static unsigned long EDX; /*used for funct_start */
static unsigned long EAX;

void _InterrupEntryPoint(void);
void _ISREntryPoint(void);
void _KeyboardEntryPoint(void);

static int rc, interrupt;


/* Purpose: allows user process to switch to kernel process (second half)
 *			and kernel process to switch to user process (first half)
 * Input: p: pointer to the process
 * Output: the type of REQ_ID. In our case {CREATE, YIELD, STOP}
 */
extern int contextswitch(struct pcb *p){
	/* check if there is a signal pending */
	int sig_num = isSignal(p);
	if (sig_num != -1){
		signal_helper(p, sig_num);
		p->sig_status[sig_num] = 0; /*reset status so someone else can call it */
	}
	/*save user process stack */
	ESP = p->esp;
	/* save process's result, might not be useful */
	EAX = p->create_result;

	__asm __volatile( " \
		pushf  \n\
		pusha  \n\
		movl  %%esp, k_stack  \n\
		movl  ESP, %%esp  \n\
		popa  \n\
		movl  EAX, %%eax  \n\
		iret  \n\
	_KeyboardEntryPoint:  \n\
		cli  \n\
		pusha  \n\
		movl $2, %%ecx  \n\
		jmp _CommonJump  \n\
	_InterrupEntryPoint:  \n\
		cli  \n\
		pusha  \n\
		movl  $1, %%ecx  \n\
		jmp  _CommonJump  \n\
	_ISREntryPoint:  \n\
		cli  \n\
		pusha  \n\
		movl  $0, %%ecx  \n\
	_CommonJump:  \n\
		movl  %%eax, rc  \n\
		movl  %%edx, EDX  \n\
		movl  %%ecx, interrupt  \n\
		movl  %%esp, ESP  \n\
		movl  k_stack, %%esp  \n\
		popa  \n\
		movl  rc, %%eax  \n\
		movl  interrupt,  %%ecx  \n\
		popf  \n\
			"
		:
		: 
		: "%eax", "%edx", "%ecx"
		);
	/* save user process stack pointer */
	p->esp = ESP;

	/* when it is a interrupt, we do not want to change any value */
	if(interrupt == 1){
		p->create_result = rc;
		rc = TIMER_INT;
	}
	else if (interrupt == 2){
		p->create_result = rc;
		rc = KEYBOARD_INT;
	}
	else{
		/* save adress of argument after req in syscallva(...) */
		p->args = EDX;
	}
	
	/* return EAX since the CALL value is loaded in here */
	return rc;
}

/* Purpose: Initialize context switcher by creating a new entry in IDT to point
 *			to the middle of it, so INT will come in the middle of context 
 *			switcher, and switch from process to kernel
 * Input: none
 * Output: none
 */
extern void contextinit(void){
	set_evec(49,(unsigned long)&_ISREntryPoint);
	set_evec(32,(unsigned long)&_InterrupEntryPoint);
	/* it is 33 since keyboard is IRQ 1 */
	set_evec(33,(unsigned long)&_KeyboardEntryPoint);	
	initPIT( TIME_SLICE );
}

/* 	PURPOSE: check if a signal is marked
	INPUT: process p
	OUTPUT: -1 if there is no signal marked
			i the highest priority signal that is marked
	NOTE: priority is enforced via high to low sig #
*/
int isSignal(struct pcb* p){
	int i ;
	int result = -1;
	for (i=SIG_TABLE_SIZE-1;i>=0;i--){
		if (p->sig_status[i] == 1){
			result = i;
			break;
		} 
	}
	return result;	
}

/* 	PURPOSE: service a user defined signal by building fake parameters
			context frames, calling sigtramp() and specific handler
	INPUT: 	process p that needs to service a signal
			real_sig_no that is the highest marked signal
	PRECONDITION: real_sig_no is marked
	OUTPUT: none
*/
void signal_helper(struct pcb* p, int real_sig_no){

    void (*handler) (void*) ;
	handler = p->sig_table[real_sig_no];
	/* set up fake user stack here before tramp*/

	/* need a old context */
	unsigned long * ESP = p->esp; 
	ESP =  ESP - 1; 
	*ESP = (unsigned long) p->esp;		
	// esp = (unsigned long *)((unsigned char )esp - 1);
	ESP =  ESP - 1; 
	*ESP = (unsigned long) handler;						
	ESP =  ESP - 1; 		
	*ESP = 0; /*fake return address */
	p->esp = ESP;

	/* need a fake context */
	struct context_frame *ctfm = (struct context_frame *)(p->esp - sizeof(struct context_frame)/sizeof(unsigned long));
	ctfm->edi = 0;
	ctfm->esi = 0;
	ctfm->ebp = 0;
	ctfm->esp = 0;
	ctfm->ebx = 0;
	ctfm->edx = 0;
	ctfm->ecx = 0;
	ctfm->eax = 0;
	ctfm->iret_eip = (unsigned long) &sigtramp;
	ctfm->iret_cs = getCS();
	ctfm->eflags = 0x00003200;
	/* update esp value */
	p->esp = (unsigned long *) ctfm;	
}