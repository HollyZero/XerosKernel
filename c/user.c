/* user.c : User processes */

#include <xeroskernel.h>
#include <xeroslib.h>
extern struct pcb *ready_queue_head; /* head of ready queue */
unsigned long TIME_TO_SLEEP = 5000;
unsigned int root_pid = 2; /*hard code because racing condition*/
unsigned int pid_example;
unsigned int curr_pid; 

extern struct pcb pcb_table[PCB_TABLE_SIZE]; //not supposed to be here, here to test

static char * valid_username = "cs415";
// static char * valid_password = "EveryoneGetsAnA";
static char * valid_password = "everyonegetsana";

static char * valid_commands[] = {
    "ps",
    "ex",
    "k",
    "a",
    "t",
    "m",
};


#define ORIGINAL
// #define TEST
// #define PRIORITY_SIGNAL_TEST
// #define SYSSIGHANDLER_TEST
// #define SYSKILL_TEST
// #define SYSSIGWAIT_TEST
// #define SYSOPEN_TEST
// #define SYSWRITE_TEST
// #define SYSSIOCTL_TEST
// #define SYSREAD_TEST
// #define TEST_1
// #define TEST_2

/* Your code goes here */
/* wrapper function so we don't need to use sysstop() 
 */
void wrapper(void (*func) (void)){
	func();
	sysstop();
}

/* idle process that will only run when there is no other process
 */
extern void idleproc(void){
	for(;;);
}

/*Example 8: Time sharing */
void root_8(void){

	syscreate(&producer,PROCESS_STACK_SIZE);
	syscreate(&consumer,PROCESS_STACK_SIZE);


}

extern void root(void){

	#ifdef TEST
	our_tests();
	#endif

	#ifdef ORIGINAL

	// for(;;);
	int flag = TRUE;
	for (;;){
		int user_bufflen = 7;
		char user_buffer[user_bufflen];	
		int pw_bufflen = 17;
		char pw_buffer[pw_bufflen];	
		do{

			/* 1. print a banner */
			sysputs("\nWelcome to Xeros - an experimental OS\n\n");

			/* 2. opens the keyboard in no echo mode */
			int fd = sysopen(KB_0);
			/* 3. turns keyboard echoing on */
			int ret = sysioctl(fd, ECHO_ON);
			/* 4. prints username */
			sysputs("Username: ");
			/* 5. reads username */
			ret = sysread(fd, user_buffer, user_bufflen); /* expecting return with enter key */
			/* 6. Turns keyboard echoing off */
			ret = sysioctl(fd, ECHO_OFF);
			/* 7. prints password */
			sysputs("Password: \n");
			/* 8. reads password */
			ret = sysread(fd, pw_buffer, pw_bufflen); /*expecting enter key */

			ret = sysclose(KB_0);
			/* 9. verify username and password */
			/* 10. 11. verification fails, go back to beginning */
			if (strncmp(user_buffer, valid_username, 5) == 0 && strncmp(pw_buffer, valid_password, 15) == 0){
				sysputs("Success! Starting Shell...\n");
				flag = FALSE;
			}
			else {
				sysputs("Wrong username/password. Please try again!");
			}
		}while (flag == TRUE);

		/* 12. create the shell program */
		int shell_pid = syscreate(&shell, PROCESS_STACK_SIZE);
		/* 13. wait for shell to exit */
		int ret = syswait(shell_pid);
	}

	#endif		
}

void shell(void){
	int bufflen = 128;
	char buffer[bufflen];
	char testbuff[bufflen];
	int end_in_and_flag = 0;
	/* open keybaord in echo mode */
	int fd = sysopen(KB_1);
	for (;;){
	/* print prompt */
		int command_len = 3; /*at most 2 letters, TODO: change "enter" key case*/
		char command[command_len];
		memset(command,'\0',command_len);

		sysputs("> ");
		/* reads the command by enter key */
		int ret = sysread(fd, buffer, bufflen);
		//kprintf("ret after sysread = %d\n",ret);
		/* split the first word */
		/* for some reason, sscanf() will not compile so I wrote my own */
		splitcommand(buffer,bufflen, command, command_len);
		/* test to see if the command ends in '&'*/
		int ap = getindex(buffer,bufflen,'&');
		//kprintf("& is at position %d\n",ap);
		//kprintf("ret  = %d\n",ret);
		if(ap == ret-2){
			end_in_and_flag = 1;
		}
		/* if the command does not exist, print command not found, go back */		
		if (isValidCommand(command, command_len) == FALSE && end_in_and_flag != TRUE){
			sysputs("Command not found\n");
			//kprintf("end_in_and_flag = %d",end_in_and_flag);
		}
		else if(!end_in_and_flag){
		/* else if command exists, create a process corresponding to that command
			remember the rocess ID until the process exits */
			int c_pid = -1;
			if(strcmp(command,valid_commands[0]) == 1){
				/* ps case */
			}
			else if(strcmp(command,valid_commands[1]) == 1){
				/* ex EOF case*/
			}
			else if(strcmp(command,valid_commands[2]) == 1){
				/* k */
			}
			else if(strcmp(command,valid_commands[3]) == 1){
				/* a */
			}
			else if(strcmp(command,valid_commands[4]) == 1){
				/* t */
			}

		/* otherwise wait for command to finis with syswait() */
			syswait(c_pid);
		}
		memset(command,'\0',command_len);
		end_in_and_flag = 0;	
	}
	int ret = sysclose(KB_1);
}

/* split the command null-terminated string */
void splitcommand(char * buffer, int bufflen, char *command, int command_len){
	int i = getindex(buffer, bufflen, ' ');
	int j = getindex(buffer, bufflen, '\n');
	// kprintf("i=%d, j=%d", i,j);
	if (i <= command_len-1 || j <= command_len-1){
		if (i < j){
			strncpy(command, buffer, i);
		}
		else {
			strncpy(command, buffer, j);				
		}
		if (command[i-1] == '\n'){
			kprintf("i'm here");
			command[i-1] = '\0';
		}
		command[i+1] = '\0';		
	}
}
/* helper to split the first word */
int getindex(char* buffer, int bufflen, char c){
	int i;
	for (i=0; i<bufflen; i++){
		if (buffer[i] == c){
			break;
		}
	}
	return i;
}

/* helper to check if command is valid */
int isValidCommand(char *command, int command_len){
	int i;
	int n_commands = sizeof (valid_commands) / sizeof (char *);
	/* defined in xeroskernel.h */
	for (i=0;i< n_commands; i++){
		if (strcmp(command, valid_commands[i]) == 0){
			return TRUE;
		}
	}
	return FALSE;
}

void child_two(void){
	/*hard code 2 is PID for root */
	int result = -5;
	result = syskill(2,8);
	char buffer[1024];
	
	sprintf(&buffer[0], "child_two r=%d-------\n",result);
	sysputs(buffer);	

	for(;;); /* so root will wait forever */
}

void example_handler(void* c){
	char buffer[1024];
	
	sprintf(&buffer[0], "Example handled!\n");
	sysputs(buffer);
}

void wait_func(void){
	for (int i =0; i< 500000; i++);
	int result = -5;
	result = syskill(2,8);
	char buffer[1024];
	
	sprintf(&buffer[0], "child_one r=%d-------\n",result);
	sysputs(buffer);		
  // int i;
  // for (i = 0; i < 1000; i++);
}

void handler_kill(void * c){
	sysstop();
}

/*holdes all the test cases */
void our_tests(void){	

  	/*----------------------------------------------*/
	#ifdef PRIORITY_SIGNAL_TEST
	int pid_a;
	/*create 3 processes */
	pid_a = syscreate(&func_a, PROCESS_STACK_SIZE);
	curr_pid = pid_a; /*set up global variable */
	syscreate(&func_b, PROCESS_STACK_SIZE);
	syscreate(&func_c, PROCESS_STACK_SIZE);
	#endif

  	/*----------------------------------------------*/
	#ifdef SYSSIGHANDLER_TEST
	int ret; 
	unsigned long old_handler_byte = 8;
  	void (**old_handler) (void*)  = (void (**) (void *))old_handler_byte;	
 	char buff[512];

	sysputs("Showing that invalid signal number will fail\n");
  	ret = syssighandler(67, &example_handler, old_handler);
  	if (ret  == -1){
  		sprintf(buff, "ERROR: signal number %d invalid\n",67);
  		sysputs(buff);
  	}
  	else {
  		sysputs("This line is not supposed to be printed");
  	}	
	#endif

  	/*----------------------------------------------*/
	#ifdef SYSKILL_TEST
	int ret;
	char buff[512];

	sysputs("Showing that syskill invalid pid will fail\n");
	ret = syskill(5,8); /* 5 does not exists */
	if (ret == -712){
		sprintf(buff, "ERROR: pid %d is invalid\n",5);
		sysputs(buff);
	}
  	else {
  		sysputs("This line is not supposed to be printed");
  	}	
	#endif

  	/*----------------------------------------------*/
	#ifdef SYSSIGWAIT_TEST
	int pid_a, pid_b;

	pid_a = syscreate(&func_d, PROCESS_STACK_SIZE);
	curr_pid = pid_a; /*set up global variable */
	pid_b = syscreate(&func_e, PROCESS_STACK_SIZE);	
	#endif

	/*----------------------------------------------*/
	#ifdef SYSOPEN_TEST
	int ret;
	char buff[512];
	
	sysputs("Sysopen() should fail with invalid device number\n");
	ret = sysopen(10);
	sprintf(buff, "sysopen failed # %d\n", ret);
	sysputs(buff);
	#endif

  	/*----------------------------------------------*/	
	#ifdef SYSWRITE_TEST
	int ret;
	char buff[512];

	sysputs("Syswrite() should fail with invalid fd\n");
	ret = sysopen(KB_1); /*open a valid keyboard */
	if (ret >=0 && ret <=3){
		/* open up properly */
		/* try with invalid file descriptor */
		ret = syswrite(4, buff, 3);
		if (ret == -2){
			sysputs("ERROR: syswrite() with invalid file descriptor\n");
		}
  		else {
  			sysputs("This line is not supposed to be printed");
  		}			
	}
	else {
  		sysputs("This line is not supposed to be printed");
  	}	
	#endif

  	/*----------------------------------------------*/	
	#ifdef SYSSIOCTL_TEST
	int ret, fd;

	sysputs("Sysioctl() with invalid command\n");
	fd = sysopen(KB_1);
	/* bunch of invalid command and arguments */
	ret = sysioctl(fd, 90, 0, 12, 33);
	if (ret == -1 ){
		sysputs("ERROR: sysioctl() invalid command\n");
	}
	else {
  		sysputs("This line is not supposed to be printed");		
	}

	#endif

  	/*----------------------------------------------*/	
	#ifdef SYSREAD_TEST
	int ret, fd; 
 	char buff[512];

	sysputs("sysread() with more characters buffered in kernel\n");
	char buffer[128];
	int bufflen = 3;

	fd = sysopen(KB_1);
	syssleep(10000);
	/* enter 4 characters right away so kernel will have buffer full*/
	ret = sysread(fd,buff, bufflen);

	sysputs("\nUser reads 3 characters: ");
	int i;
	for (i=0; i < bufflen; i++){
		sprintf(&buffer[i], "%c",buff[i]);
	}
	sysputs(buffer);
	sprintf(buffer,"sysread ret= %d", ret);
	sysputs(buffer);
	#endif

  	/*----------------------------------------------*/	
	#ifdef TEST_1
	#endif

  	/*----------------------------------------------*/	
	#ifdef TEST_2
	#endif

  	/*----------------------------------------------*/	
	// UNIT TEST: sysgetcputimes()
	// processStatuses psTab;
	// int procs;
	// syssleep(500);
	// procs = sysgetcputimes(&psTab);

	// for(int j = 0; j <= procs; j++) {
	//   sprintf(buffer, "%4d    %4d    %10d\n", psTab.pid[j], psTab.status[j], 
	//   psTab.cpuTime[j]);
	//   sysputs(buffer);
	// }	
}

/* test PRIORITY_SIGNAL_TEST */
void func_a(void){
  	sysputs("Process A starts\n"); 	
	/*install two handler */
 	unsigned long old_handler_byte = 8;
  	void (**old_handler) (void*)  = (void (**) (void *))old_handler_byte;
  	int ret;
  	ret = syssighandler(8, &example_handler, old_handler);
  	if (ret < 0){
  		sysputs("sighandler 8 installization failed\n");
  	}
  	ret = syssighandler(9, &handler_kill, old_handler);
  	if (ret < 0){
  		sysputs("sighandler 8 installization failed\n");
  	}
	/*sleep for a while, so when it wake up 9 and 8 are both signaled*/  	
  	syssleep(7000);
  	sysputs("This line should not be printed\n");   		
}
/* test PRIORITY_SIGNAL_TEST */
void func_b(void){
	/* sends a signal 8 to process A (pid global variable)*/
	char buff[128];
	int result;
  	sysputs("Process B starts\n");
  	syssleep(4000); /*so process A can install its handlers */
	result = syskill(curr_pid,8);
	sprintf(buff,"Process B: syskill result=%d\n",result);
	sysputs(buff);
}
/* test PRIORITY_SIGNAL_TEST */
void func_c(void){
	/* sends a signal 9 to process A */
	char buff[128];
	int result;
  	sysputs("Process C starts\n");
  	syssleep(4000); /*so process A can install its handlers */  	 
	result = syskill(curr_pid,9);
	sprintf(buff,"Process C: syskill result=%d\n",result);
	sysputs(buff);	
}

/* test SYSSIGWAIT_TEST */
void func_d(void){
	sysputs("Process A starts\n");
	/*going to sleep*/
	syssleep(100000);
	sysputs("Process A: Woke up, going to die...\n");
}
/* test SYSSIGWAIT_TEST */
void func_e(void){
	// char buffer[32];
	sysputs("Process B starts, going to wait for Process A to die\n");
	/* try to wait for Process A */
	int r = syswait(curr_pid);
	/* our implementation choice is that only wait for processes 
	   which are on the ready queue */
	if (r == -2){
		sysputs("Stop waiting for Process A");
		sysputs("Process A is not on the ready queue\n");
	}
  	else {
  		sysputs("This line is not supposed to be printed");
  	}		
}

/* Purpose: the initial process to be run 
 *			If on its own, it will just sysyield itself and
 *			do nothing
 * Input: none
 * Output: none
 */
extern void root_9 (void){
	root_pid = sysgetpid();
	sysputs("\nroot: I'm alive");
	int i;
	int MAX_PROCESS = 4;
	int pid[4];
	char buffer[1024];

	for (i = 0; i < MAX_PROCESS; i++){
		pid[i] = syscreate(&receiver, PROCESS_STACK_SIZE);
		sprintf(&buffer[0], "Root has created a process with PID= %d",pid[i]);
		sysputs(buffer);		
	}

	syssleep(4000);
	syssend(pid[2],7000);
	syssend(pid[1],7000);
	syssend(pid[0],7000);
	syssend(pid[3],7000);

	unsigned int *from_pid = (unsigned int *) &pid[3];
	unsigned long* num = NULL;	

	int status = sysrecv(from_pid,num);
	sprintf(&buffer[0], "\nreturn status of attempt to receive a message fom the 4th process is %d",status);
	sysputs(buffer);

	int num_elapse = 3000;
	status = syssend(pid[2],num_elapse);
	sprintf(&buffer[0], "\nreturn status of attempt to send a message fom the 3th process is %d",status);
	sysputs(buffer);	
	sysstop();
}

/* can only be a void in parameter here, haven't figured out how */
void receiver(void){
	char buffer[1024];
	unsigned long holderrr;
	unsigned long *num = &holderrr;
	int pid = sysgetpid();

 	sprintf(&buffer[0],"\nRecv: I'm alive: %d", pid);
 	sysputs(buffer);	
	syssleep(5000);
	unsigned int holdeeee;
	unsigned int * from_pid = &holdeeee;
	*from_pid = root_pid;
 	// sprintf(&buffer[0],"in user receive: %d", *from_pid);
 	// sysputs(buffer);
 	int recv_result = sysrecv(from_pid, num);
 	if (recv_result == 0){
 		sprintf(&buffer[0],"\nMessage received: specified to sleep in milliseconds: %d", *num);
 		sysputs(buffer);
		syssleep(*num);
  		sysputs("\nSleeping has stopped. Exit...");
 	}
}

/* Purpose: this process print "Happy 101st" and yields 12 times 
 * Input: none
 * Output: none
 */
void producer(void){
	int PRODUCER_SIZE = 1200;
	int i;
	char buffer[1024];
	
	for(i = 0; i < PRODUCER_SIZE; i++){
		sprintf(&buffer[0], "Happy \t %d",i);
		//sysputs(buffer);
	 	//sysyield();
	}
	//sysstop();
}

/* Purpose: this process print "Birthday UBC" and yields 15 times
 * Input: none
 * Output: none
 */
void consumer(void){
	int CONSUMER_SIZE = 15;
	int i;
	char buffer[1024];

	
	for(i=0;i < CONSUMER_SIZE; i++){
		sprintf(&buffer[0], "Birthday UBC \t %d",i);
		sysputs(buffer);
	}
	//sysstop();	
}