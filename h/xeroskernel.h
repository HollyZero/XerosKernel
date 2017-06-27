/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout Xinu */

typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes 
                              * addressable in this architecture.
                              */
#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */


/* Universal return constants */

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);
void           set_evec(unsigned int xnum, unsigned long handler);

/*	part 7.2 - Memory Management*/
extern void kmeminit(void);
extern void *kmalloc(int size);
extern void kfree(void *ptr);

/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);
typedef void (*fancyfuncptr) (void (*) (void));
typedef void (*fancierfuncptr) (void (*) (void*));
typedef void (*morefancierfuncptr) (void (**) (void*));

/* 7.2 memory header from slide */
struct memHeader {
	unsigned long size;
	struct memHeader *prev;
	struct memHeader *next;
	char *sanityCheck;
	unsigned char dataStart[0];
};

/* constant values used for request types, process states*/
#define CREATE 1
#define YIELD 2
#define STOP 3
#define READY 4
#define RUNNING 5
#define IDLE 13
/* assignment 2 */
#define GETPID 6
#define PUTS 7
#define KILL 8
#define SEND 9
#define RECV 10
#define BLOCK 11
#define TIMER_INT 12
#define SLEEP 14
/* assignment 3 */
#define CPUTIME 15
#define SETSIGNAL 16
#define SIGRETURN 17
#define WAIT 18
#define OPEN 19
#define CLOSE 20
#define WRITE 21
#define READ 22
#define IOCTL 23
#define KEYBOARD_INT 24
#define BLOCK_PROC -3
/*device #*/
#define CONSOLE    0
#define SERIAL0    1
#define SERIAL1    2
#define KBMON      3
#define TTY0       4 /*from slide but won't be used*/
/*they are the same # so only 1 of them can be opened at a time*/
#define KB_0 0
#define KB_1 1 /*actual assignment*/
/*ioctl */
#define SET_EOF 53
#define ECHO_OFF 55
#define ECHO_ON 56

/* static values*/
#define PROCESS_STACK_SIZE 16384
#define SAFETY_MARGIN 16
#define PCB_TABLE_SIZE 64
#define SIG_TABLE_SIZE 32
#define SANITY_VALUE 78638
#define TIME_SLICE 100
#define MILLISECONDS_TICK 10
#define DEVICE_TABLE_SIZE 2
#define FD_TABLE_SIZE 4
#define BUFFER_SIZE 32
#define KKB_BUFFER_SIZE 4


/*fd_table should be a struct such as: from slide 334 */
struct struct_fdt{
	int major;
	int minor;
	struct devsw * dev_p;
	void *ctr_data;
	int status;
	// char[32] name; /*TODO is this how you initialize a block for a string? */
};

/* PCB structure */
struct pcb {
	int PID;	/*process ID*/
	int state;		/*process state:READY,RUNNING,STOP*/
	struct pcb *next;	/* pointer to the next pcb on queue*/
	unsigned long* esp;	 	/* user space stack pointer */
	unsigned long* kmem_space;/* address of starting memory of the process*/
	long args;
	int create_result;	/* creation result from dispatcher */
	struct pcb *block_q_sender;
	struct pcb *block_q_receiver;
	unsigned long buf;
	unsigned long s_num;/*sender save num here when block*/
	unsigned long *u_p_num;/*saveing user num pointer on receiver pcb*/
	unsigned int tick;
	int cpu_time;
	void (*sig_table[SIG_TABLE_SIZE]) (void*);
	//struct sig_entry sig_table[SIG_TABLE_SIZE];
	struct pcb *block_q_wait;/* TODO:add to queue only if waiting target is not blocked*/
	int sig_status[SIG_TABLE_SIZE];
	struct pcb *waitingForPCB; 
	struct struct_fdt fd_table[FD_TABLE_SIZE];
	unsigned char* u_buff;
	int u_buff_len;
	int keyboard_fd; /*init.c and create.c */

} ;

typedef struct struct_ps processStatuses;
struct struct_ps {
  int  pid[PCB_TABLE_SIZE];      // The process ID
  int  status[PCB_TABLE_SIZE];   // The process status
  long  cpuTime[PCB_TABLE_SIZE]; // CPU time used in milliseconds
};


/* context frame structure,with all register and flag values*/
struct context_frame{
	unsigned long edi;
	unsigned long esi;
	unsigned long ebp;
	unsigned long esp;
	unsigned long ebx;
	unsigned long edx;
	unsigned long ecx;
	unsigned long eax;
	unsigned long iret_eip;
	unsigned long iret_cs;
	unsigned long eflags;

};

/*device table structure */
struct devsw {
	int dvnum;
	char *dvname;
	int (*dvinit)(void*);
	int (*dvopen)(void*, int);
	int (*dvclose)(void*, int);
	int (*dvread)(void*, int,void (*)(void*,int, int),void*,int );
	int (*dvwrite)(void*, int);
	int (*dvseek)(void*);
	int (*dvgetc)(void*);
	int (*dvputc)(void*);
	int (*dvcntl)(void*);
	void *dvcsr;
	void *dvivec;
	void *dvovec;
	int (*dviint)(void*);
  	int (*dvoint)(void*);
  	void *dvioblk;
	int dvminor;
};

struct devsw devtab[DEVICE_TABLE_SIZE];
/* kernel side keyboard buffer for non-echo kb*/
unsigned int kkb_buffer[KKB_BUFFER_SIZE];

/*dispatcher*/
extern void dispatch(void);
struct pcb* next(void);
extern void ready(struct pcb* process);
void cleanup(struct pcb* process);

/* context switcher*/
extern void contextinit(void);
extern int contextswitch(struct pcb *p);

/* system call */
extern int syscall3(int call , void (*func) (void),int stack);
extern int syscall1(int call);
extern int syscallva(int call, ...);
extern unsigned int syscreate(void (*func) (void),int stack);
extern void sysyield(void);
extern void sysstop(void);

/* create */
extern int create(void (*func) (void (*) (void)),void (*real_func) (void),int stack);

/* user processes */
extern void root (void);
void producer(void);
void consumer(void);
void wrapper(void (*func) (void));
void receiver(void);

/* helper private functions */
void test_disp(void);
extern void print_queue(struct pcb *p);
extern void printMemList (void);

/* private test functions */
void test1(void);
void test2(void);
void test3(void);
void test4(void);

/* Anything you add must be between the #define and this comment */
/* Assignment 2 */
/*syscall*/
extern int sysgetpid( void );
extern void sysputs( char *str );
extern int syskill( int pid ,int signalNum);
extern struct pcb* findprocess(int pid);

extern int syssend(int dest_pid, unsigned long num);
extern int sysrecv(unsigned int *from_pid, unsigned long *num);
extern unsigned int syssleep( unsigned int milliseconds );


/* A2 3.4 send and receive */
extern int send(int destPID, struct pcb* srcPCB, unsigned long num);
extern int receive(int *srcPID, struct pcb* destPCB, unsigned long* len);
int isInQ(struct pcb* head, struct pcb* curr);
extern struct pcb * enqueue(struct pcb* head, struct pcb* curr, int state);
extern struct pcb * reconsQ(struct pcb* head, struct pcb* curr);
extern void reconsReadyQ(struct pcb* curr);

/* A2 3.5 idle */
extern void idleproc(void);
void pcb_table_init(void);

/* 3.7 sleep */
extern void sleep(struct pcb* p);
extern void tick_function(void);
void adjustTicks(struct pcb* head, unsigned int num);
struct pcb * findPosition(struct pcb* head, unsigned int num);
extern unsigned int prev_sum(struct pcb *curr);
void memorial_service(struct pcb* p);

extern void print_pcb_state(void );

/*A2 */
void root_1(void);
void receiver_1(void);
void root_2(void);
void receiver_2(void);
void root_3(void);
void receiver_3(void);
void root_4(void);
void receiver_4(void);
void root_5(void);
void root_6(void);
void root_7(void);
void root_8(void);

/* A3*/
/*syscall.c*/
extern int sysgetcputimes(processStatuses * ps);
extern int syssighandler(int sig_num, void (*newhandler) (void*), void (**oldHandler) (void*));
extern void syssigreturn(void* old_sp);
extern int syswait(int PID);
extern int sysopen(int device_no);
extern int sysclose(int fd);
extern int syswrite(int fd, void* buff, int bufflen);
extern int sysread(int fd, void* buff, int bufflen);
extern int sysioctl(int fd,unsigned long command,...);


/*dispatch.c*/
int getcputime(int pid);
int getCPUtimes(struct pcb *p, processStatuses *ps);
int kill(struct pcb* currP, int pid, int signal_number);


/*signal.c*/
extern int sighandler(struct pcb* p,int sig_id, void (*newhandler) (void*), void (**oldHandler) (void*));
extern int isValid(int sig_id);
int isValidAddress(void (*handler)(void*));
extern int p2pwait(int dest_pid,struct pcb* p);
extern int signal(int pid, int sig_no);
extern void sigtramp(void (*handler)(void*), void* cntx);

/*user.c */
extern void root_9(void);
void example_handler(void* c);
void wait_func(void);
void handler_kill(void* c);
void child_two(void);
/* user.c for testing purposes */
void our_tests(void);
void func_a(void);
void func_b(void);
void func_c(void);
void func_d(void);
void func_e(void);
void shell(void);
int getindex(char* buffer, int bufflen, char c);
int isValidCommand(char *command, int command_len);
void splitcommand(char * buffer, int bufflen, char *command, int command_len);

/*ctsw.c */
void signal_helper(struct pcb* p, int real_sig_no);
int isSignal(struct pcb* p);

/*di_calls.c*/
extern int di_open(struct pcb* p, int device_no);
extern int di_close(struct pcb* p, int device_no);
extern int di_write(struct pcb* p, int fd, void* buff, int bufflen);
extern int di_read(struct pcb* p, int fd, void* buff, int bufflen,int count);
extern int di_ioctl(struct pcb* p, int fd, void *  command_addr);
int isInRange(int num, int max, int min );
extern void unblock_read(struct pcb* p, int len,int count);

/*init.c */
void init_entry(int i);
void devtab_init(void);

/*sleep.c */
extern struct pcb * reconsSleepQ(struct pcb* p);

#endif
