/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>
#include <kbd.h>

#define REALCODE 
//#define TEST_SEND_1
//#define TEST_SEND_2
//#define TEST_RECEIVE_1
//#define TEST_RECEIVE_2
//#define TEST_SEND_FAIL
//#define TEST_RECEIVE_FAIL_1
//#define TEST_RECEIVE_FAIL_2
//#define TIME_SHARE_TEST
//#define TEST_SYSGETCPUTIME

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/
struct pcb *ready_queue_head; /* head of ready queue */
struct pcb *ready_queue_tail; /* tail of ready queue */
struct pcb *sleep_queue;
//struct pcb *blocked_queue_head; /* head of blocked queue */
//struct pcb *blocked_queue_tail; /*tail of blocked queue */
struct pcb *stopped_queue; /* head of stopped queue */
struct pcb *receiveFromAnyQ;
struct pcb* idle_proc; 
int p_count; /* keep track of pid globally */
struct pcb pcb_table[PCB_TABLE_SIZE]; /* process table */
unsigned long device_g_buffer[DEVICE_TABLE_SIZE][BUFFER_SIZE]; /* to use as buffer */
/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc( void )				/* The beginning */
{
  
  char str[1024];
  int a = sizeof(str);
  int b = -69;
  int i; 

  kprintf( "\n\nCPSC 415, 2016W1 \n32 Bit Xeros 0.01 \nLocated at: %x to %x\n", &entry, &end); 
  

  /* Build a string to print) */
  sprintf(str, 
      "This is the number -69 when printed signed %d unsigned %u hex %x and a string %s.\n      Sample printing of 1024 in signed %d, unsigned %u and hex %x.\n",
	  b, b, b, "Hello", a, a, a);

  for (i = 0; i < 1000000; i++);

  /* initialize memory manager lists */
   kmeminit();

  /* initialize process table, able to contain 
  at least 32 processes, power of 2*/

  /* initialize process queues and each processes*/
  
  p_count = 1; /*process ID counter, increase for each process */
  // kprintf("init: creating root 2-----");  
  pcb_table_init();
  devtab_init();

  /*ready queue is initially empty*/
  ready_queue_head = NULL;
  ready_queue_tail = NULL;

  /*  initialize interrupt table */
  contextinit();
  
  /* create idle process, we know it is PCB[0] 
	 and it is not on any queue for now
   */
  create(&wrapper, &idleproc, PROCESS_STACK_SIZE);
  idle_proc = &pcb_table[0];   
 
  /* create root process*/
  create(&wrapper, &root, PROCESS_STACK_SIZE);

  dispatch();
  /* Add all of your code before this comment and after the previous comment */
  /* This code should never be reached after you are done */
  kprintf("\n\nWhen the kernel is working properly ");
  kprintf("this line should never be printed!\n");
  for(;;) ; /* loop forever */
}

/* initialize table and put every process on stopped queue, 
  stopped queue is used to indicate free pcb entries*/
void pcb_table_init(void){

  int i; 

  init_entry(0);
  init_entry(1);
  stopped_queue = &pcb_table[1]; 
  
  struct pcb* tmp = stopped_queue;
  for (i =2; i < PCB_TABLE_SIZE; i++){
    init_entry(i);
    tmp->next = &pcb_table[i];
    tmp = tmp->next;
  }
  tmp->next = NULL;
}

void init_entry(int i){
    int j;

    pcb_table[i].PID = -1;
    pcb_table[i].state = STOP;
    pcb_table[i].esp = NULL;
    pcb_table[i].kmem_space = NULL;
    pcb_table[i].args = 0;  
    pcb_table[i].create_result = 0;
    pcb_table[i].block_q_sender = NULL;
    pcb_table[i].block_q_receiver = NULL;  
    pcb_table[i].buf = 0;
    pcb_table[i].s_num = 0;
    pcb_table[i].tick = 0;
    pcb_table[i].u_p_num = NULL;
    pcb_table[i].cpu_time = 0;
    for(j = 0; j<SIG_TABLE_SIZE; j++){
      pcb_table[i].sig_table[j] = NULL;
      pcb_table[i].sig_status[j] = 0;
    }
    pcb_table[i].block_q_wait = NULL; 
    pcb_table[i].waitingForPCB = NULL;
    for (j=0; j<FD_TABLE_SIZE;j++){
      pcb_table[i].fd_table[j].major = -1;
      pcb_table[i].fd_table[j].minor = -1;
      pcb_table[i].fd_table[j].dev_p= NULL;
      pcb_table[i].fd_table[j].ctr_data= NULL;
      pcb_table[i].fd_table[j].status= 0;                  
    }
    pcb_table[i].u_buff = NULL;
    pcb_table[i].u_buff_len = 0;
}

void devtab_init(void){

  /* only got 2 devices, so I can use a loop */
  int i;
  for (i=0;i<DEVICE_TABLE_SIZE; i++){
    devtab[i].dvnum = KBMON; /*major */
    devtab[i].dvopen = (int (*)(void*, int))&kbdopen;  
    devtab[i].dvclose = (int (*)(void*, int))&kbdclose;  
    devtab[i].dvinit = NULL;
    devtab[i].dvwrite = (int (*)(void*, int))&kbdwrite;  
    devtab[i].dvread = (int (*)(void*, int, void (*)(void*,int, int),void*, int))&kbd_read;    
    devtab[i].dvseek = NULL;
    devtab[i].dvgetc = NULL;
    devtab[i].dvputc = NULL;
    devtab[i].dvcntl = (int (*)(void*))&kbdioctl;
    devtab[i].dvcsr = NULL;
    devtab[i].dvivec = NULL;
    devtab[i].dvovec = NULL;
    devtab[i].dviint = NULL;
    devtab[i].dvoint = NULL;
    devtab[i].dvioblk = (void *)device_g_buffer+i*BUFFER_SIZE;   
    devtab[i].dvminor = i;
  }

}