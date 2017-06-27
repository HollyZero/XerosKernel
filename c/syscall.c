/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <stdarg.h>

/* Your code goes here */
/* Need to create a trap in here. */

/*
 Purpose:	system call for 3 arguments, using registers 
			to store and pass user input parameters.
 Input:		call - indicates system call type,either CREATE,YIELD,STOP
 Output: 	the result of the system call, either success or fail.
 */
extern int syscall3(int call , void (*func) (void),int stack){

	/*local variable to store the value passed back from %eax*/
	int c_result = 0;

	/*write input parameters into %eax,%edx and %ecx*/
	/*retrieve return value from %eax to c_result*/
	__asm __volatile( " \
		movl  %1, %%eax  \n\
		movl 12(%%ebp), %%edx  \n\
		movl 16(%%ebp), %%ecx  \n\
		int  $49  \n\
		movl  %%eax, %0  \n\
			"
		: "=r"(c_result)			
		: "r"(call)
		: "%eax"
		);

	return c_result;

}

/* 	Purpose: system call for various length of arguments
	Input: req: the integer representing the request for dispatcher
		   ...: the rest of the arguments, passed in using EDX as
				starting address
	Output: return value of system call indicating something
*/
extern int syscallva(int req, ...){

    va_list     ap;
    int         rc;

    va_start( ap, req );

    __asm __volatile( " \
        movl %1, %%eax \n\
        movl %2, %%edx \n\
        int  $49 \n\
        movl %%eax, %0 \n\
        "
        : "=g" (rc)
        : "g" (req), "g" (ap)
        : "%eax" 
    );
 
    va_end( ap );

    return( rc );	
} 

/*
 Purpose:	system call for 1 arguments, using registers 
			to store and pass user input parameters.
 Input:		call - indicates system call type,either CREATE,YIELD,STOP
 Output: 	the result of the system call, either success or fail.
 */
extern int syscall1(int call){
	/*local variable to store the value passed back from %eax*/
	int c_result = 0;

	/*write input parameters into %eax*/
	/*retrieve return value from %eax to c_result*/
	__asm __volatile( " \
		movl  %1, %%eax  \n\
		int  $49  \n\
		movl  %%eax, %0  \n\
			"
		: "=r"(c_result)			
		: "r"(call)
		: "%eax"
		);

	return c_result;

}

/*
 Puepose:	system call for user to use,creates a new process
 			with specified function name.
 Input:		func - function pointer to desired function
 			stack - Fixed process size,defined to 8k
 Output: 	integer value that indicates if the creation is successful
*/
extern unsigned int syscreate(void (*func) (void),int stack){

	/*uses the system call with 3 arguments*/
	return syscallva(CREATE,&wrapper, *func,stack);

}

/* 
 Purpose:	system call for user to yield the current process
 Input:
 Output:
*/
extern void sysyield(void){

	syscallva(YIELD);

}

/* 
 Purpose:	system call for user to stop the current process
 Input: none
 Output: none 
*/
extern void sysstop(void){

	syscallva(STOP);

}

/*
 Purpose:	system call for user to get the PID of current process
 Input: none
 Output: PID
 */
extern int sysgetpid( void ){
	return syscallva(GETPID);
}

/*
 Purpose:	system call for user to print a message without being
			interrupted
 Input: str: the message to be printed
 Output: none
 */
extern void sysputs( char *str ){
	syscallva(PUTS, str);
}

/*
 Purpose:	system call to deliver a signal to a target process
 Input: pid: the PID that identifies which process to signal
 		signalNumber - number of signal to be delivered
 Output: 0 if it is successful
		 -712 if the target process does not exist
		 -651 if the signal number is invalid
 */
extern int syskill( int pid ,int signalNum){
	return syscallva(KILL, pid,signalNum);
}

/*
 Purpose:	system call for a process to send a number to 
			another process
 Input: dest_pid: identifes who is the receiver
		num: the number to be sent to the receiver
 Output: 0 if it is successful,
		 -1 if the receiver is invalid
		 -2 if trying to send to itself
		 -3 other errors
 */
extern int syssend(int dest_pid, unsigned long num){
	return syscallva(SEND, dest_pid,num);
}

/*
 Purpose:	system call for a process to request a number
			from another process
 Input: from_pid: a pointer that contains the PID which
				  identifies the requested sender
				  PID = 0 indicates receive from anyone 
		num: a pointer that will contain the number which
			 will be sent by sender to this receiver
			 if the message passing is successful
 Output: 0 if it is succssful
		 -1 if the sender is invalid
		 -2 if the receiver is requesting info from itself
		 -3 other errors
 */
extern int sysrecv(unsigned int *from_pid, unsigned long *num){
	return syscallva(RECV, from_pid, num);

}

/*
 Purpose:	system call for a process to put itself on the sleep
			queue for a number of milliseconds
 Input: milliseconds: number of milliseconds to be slept 
 Output: returns 0 when the timer is fulfilled
		 in A3 will return the amount of time it still has to wait until fulfilled
 */
extern unsigned int syssleep( unsigned int milliseconds ){
	return syscallva(SLEEP, milliseconds);
}

/*
 Purpose:	this function palces into a structure being pointed to
 			information about each current active process.
 Input:		ps - a pointer to a processStatus structure filled with
 			information about all processes currently active.
 Output: 	int value greater than 0 - last slot used.
 			-1 - if address is in HOLE
 			-2 - if structure provided goes beyond memory
 			*/
extern int sysgetcputimes(processStatuses * ps){

	return syscallva(CPUTIME,ps);

}

/*
 Purpose:	system call that registers provided function handler for
			current process at the specifed signal entry.
 Input:		signal - signal number
 			newhandler - function pointer to register on signal table
 			oldHandler - pointer to old function pointer in the signal
 						table to save it.
 Output:	-1 - signal number is invalid
 			-2 - newhandler is in invalid address
 			0 - if handler is successfully installed
*/
extern int syssighandler(int sig_num, void (*newhandler) (void*), void (**oldHandler) (void*)){
	return syscallva(SETSIGNAL,sig_num,newhandler,oldHandler);
}

/*
 Purpose:	set the process stack pointer to the provided value
 Input:		old_sp - pointer address pointing to process stack before 
 					trampoline
 Output:
*/
extern void syssigreturn(void* old_sp){
	syscallva(SIGRETURN,old_sp);
}

/*
 Purpose:	system call that causes the calling process to wait for 
 			process specified by PID to terminate.
 Input: 	PID - the pid of the process that is being waited
 Output:	0 - the call ternimates normally
 			-1 - process specifed by PID does not exist
 			-2 - current process is being signaled while waiting
*/
extern int syswait(int PID){
	return syscallva(WAIT,PID);
}

/* PURPOSE: open up the device by its device_no
 * INPUT: device_no: a major number that identifies a device
 * OUTPUT: file descriptor 0-3 if succeeds
 		   -1 if failed
 		   -2 tried to open a device thats already open
 		   -3 open other minor
 */
extern int sysopen(int device_no){
	return syscallva(OPEN,device_no);
}

/*
 Purpose:	system call that closes the provided file descriptor
 Input:		fd - file descriptor from a previously successful open
 Output:	0 - successfully closed
 			-1 - error
*/
extern int sysclose(int fd){
	return syscallva(CLOSE,fd);
}

/*
 Purpose:	system call that writes to the device associated with
 			the provided file descriptor
 Input:		fd - file descriptor that specifies the device
 			buff - data source buffer to be written
 			bufflen - specifies how many bytes to write
 Output:	number of bytes wirtten -  if successful
 			-1 - on error
 			-2 - if fd is not valid
*/
extern int syswrite(int fd, void* buff, int bufflen){
	return syscallva(WRITE,fd,buff,bufflen);
}

/*
 Purpose:	system call that reads up to a specifed number of bytes
 			from the previously opened device
 Input:		fd - file descriptor of the device to read from
 			buff - area pointed to store the data read
 			bufflen - specifies number of bytes to read
 Output:	number of bytes read - if read is successful
 			0 - if EOF was encountered
 			-1 - other errors
*/
extern int sysread(int fd, void* buff, int bufflen){
	return syscallva(READ,fd,buff,bufflen);	
}

/*
 Purpose:	system call that takes a file descriptor and executes
 			specified control command.
 Input:		fd - file descriptor of specified device
 			command - device specific commands
 Output:	-1 - error
 			0 - otherwise
*/
extern int sysioctl(int fd,unsigned long command,...){

	unsigned long * command_addr = &command;  	

    return syscallva(IOCTL,fd,command_addr);		
}