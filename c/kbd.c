/*kbd.c keybarods specific calls */

#include <kbd.h>
#include <scancodesToAscii.h>
#include <i386.h>
#include <stdarg.h>
#include <xeroskernel.h>
#include <xeroslib.h>

static unsigned char VALUE; /*used to store value read from keyboard every interrupt */
unsigned int ascii_value;
unsigned int minor;/*minor device number*/
unsigned int major;/*major device number*/
int EOF_flag;/* indicates if EOF key was pressed*/
int kbuff_head = -2;/*circular q head, remove from here*/
int kbuff_tail = -1;/*circular q tail, add from here*/
void (*call_back)(void*,int,int) = NULL;/* function pointer passed in from kbd_read*/
struct pcb* argP = NULL;/* process pointer passed in from kbd_read*/

char* user_buffer = NULL;/* user buffer pointer*/
int user_buff_len = 0; /* bytes the user wants to read*/
int trans_count = 0;/*current position in user buffer,number of bytes read*/

unsigned char EOF_MASK = 0x04; /*used for ioctl 53 */

/*
 Purpose:	enable keyboard, set global variables.
 Input:		p - process that wants to open the keyboard
 			fd_no - file descriptor value to get major minor number
 Output:	0 - always successful
*/
extern int kbdopen(struct pcb* p,  int fd_no){
	/*write 0xAE to port 64 to enable keyboard */	
    enable_irq(1, 0); /*enable IRQ 1 as keyboard */	
    /* initializes global variables*/
	major = p->fd_table[fd_no].major;
	minor = p->fd_table[fd_no].minor;
	return 0;
}

/*
 Purpose:	disbale keyboard, reset global variables.
 Input:		p - process that wants to open the keyboard
 			fd_no - file descriptor value
 Output:	0 - always successful
*/
extern int kbdclose(struct pcb* p,  int fd_no){
	/*write 0xAD to port 64 to disable keyboard */
	enable_irq(1,1);
	reset_global();
	return 0; 
}

/* 
 Purpose: always return -1 since write is not supported by keyboards
*/
extern int kbdwrite(void* buff, int bufflen){
	return -1; /*not supported */
}

/* PURPOSE: copy desired amount of characters from kernel buffer
			to user buffer.
 Input:		buff - user provided buffer location to write to
 			bufflen - user provided length to read
 			cb_func - call back function to unblock process if read finish
 			currP - parameter to call back function
 			count - number of chars read successfully so far
 Output:	0 - if EOF key was pressed
 			user_buff_len - if read is finished
 			BLOCK_PROC - if read did not finish yet
 			-1 - other errors 	
*/
extern int kbd_read(void* buff, int bufflen,void (*cb_func)(void*,int, int),void* currP, int count){
	/* re-assign counter value*/
	trans_count = count;

	/*set global variables if not set yet*/
	if(user_buffer == NULL){
		user_buffer = buff;
		user_buff_len = bufflen;
		call_back = cb_func;	
		argP = currP;		
	}

	/* if EOF was read, always return 0*/
	if(EOF_flag){
		return 0;
	}
	/*read as many as possible from kernel buffer*/
	save_user_buff();
	/*if reading is done, reset count and return */
	if(trans_count == user_buff_len){
		trans_count = 0;
		return user_buff_len;
	}
	/* if reading did not finish */
	else if (trans_count < user_buff_len){
		return BLOCK_PROC;
	}
	return -1;
}


/* 	PURPOSE: excute a command specific to keyboard device
	INPUT: address of command on stack
			so from that address, we can get all the other args followed
	OUTPUT: 0 if a command is successfully executed
			-1 if a command is not found
*/
extern int kbdioctl(void* command_addr){
	
	va_list ap = (va_list)command_addr;
	int command = va_arg(ap, int);
	unsigned long * outc;
	
	int ret = 0;

	switch(command){
		case SET_EOF :
			/* we asked the TA and he said we can ignore 0 just set EOF */
			outc = (unsigned long *)va_arg(ap, int);
			/*exchange the EOF_MASK value, and save the old value */
			char temp = *outc;
			*outc = EOF_MASK;			
			EOF_MASK = temp;
			/* have to press the key again to actually enter EOF */
			break;
		case ECHO_OFF :
			minor = KB_0; /* this one is off */
			break;
		case ECHO_ON :
			minor = KB_1; /* this one is on */
			break;
		default :
			ret = -1;
			break;
	}	
	return ret;
}

/*
 Purpose:	handles keyboard interrupt for both versions. If there is process 
 			waiting for input, write to user buffer, otherwise store to kernel 
 			buffer.
 Input:
 Output:
*/
void kbd_int_handler(){

	/* read from keyboard input port*/
	unsigned char in_value = read_char();
	/* if no EOF key detected*/
	if(!EOF_flag){

		/* if it's not key up*/
		if(in_value != NOCHAR && in_value != 0){

			/*if is echo version, print*/
			if(minor == KB_1){
				kprintf("%c",in_value);
			}

			/* if there is process wating*/
			if(user_buffer != NULL){			
				/* write character to user buffer*/
				user_buffer[trans_count] = (char)in_value;
				/* increment counter*/
				trans_count++;
				
				/*check if read is complete*/
				if(trans_count == user_buff_len){
					/* reset counter and user buffer*/
					trans_count = 0;
					user_buffer = NULL;
					/*unblock process*/				
					call_back(argP,user_buff_len, trans_count);

				}
				/* if ENTER detected */
				if(in_value == ENTER){
					/* saves the current count number*/
					int tmp = trans_count;
					/* reset counter and user buffer */
					trans_count = 0;
					user_buffer = NULL;
					/* unblock process */
					call_back(argP,tmp, trans_count);
				}		
			}
			else{
			/*block queue is empty,save to kernel buffer*/
				save_kchar(in_value);
			}
		}
	}
	else{/* EOF key is detected, return with 0*/
		if(call_back != NULL){
			call_back(argP,0, trans_count);			
		}
	}
}

/*
 Purpose:	reading a character from keyboard interrupt, and translate into ascii
 Input:
 Output:	ascii_value - the ascii value translated by kbroa()
*/
extern unsigned char read_char(void){
	__asm __volatile( " \
		pusha  \n\
		in  $0x60, %%al  \n\
		movb %%al, VALUE  \n\
		popa  \n\
			"
		:
		: 
		: "%eax"
		);	
	/* change the scancode back to ASCII */
	ascii_value = kbtoa(VALUE);	
	if(ascii_value == EOT || ascii_value == EOF_MASK){
		/*set EOF flag*/
		EOF_flag = TRUE;

	}

	return ascii_value;
}

/*
 Purpose:	save value read from keyboard port to kernel buffer,
 			if kernel buffer is full, ignore value
 Input:		value - ascii value to add to kernel buffer
 Output:
*/
void save_kchar(unsigned char value){

	/*if kernel buff is empty*/
	if(kbuff_head == -2 && kbuff_tail == -1){
		kbuff_head = 0;
		kbuff_tail = 0;
		/*write value in*/
		kkb_buffer[kbuff_tail]=value;
		/* increment tail pointer*/
		kbuff_tail++;
	}
	/*if kernel buffer is not full yet,add value*/
	else if(kbuff_head != kbuff_tail){
		kkb_buffer[kbuff_tail] = value;
		/* increment tail index*/
		kbuff_tail++;
		kbuff_tail = (kbuff_tail) % KKB_BUFFER_SIZE;
	}

}

/*
 Purpose:	copy entries from kernel buffer to user buffer.
 Input:
 Output:
*/
void save_user_buff(void){
	/*get how much more the process want to read*/
	int left_to_read = user_buff_len - trans_count;
	int i;
	/* for each desire byte to read*/
	for (i = 0; i < left_to_read; i++){
		/* if kernel buffer is not empty*/
		if(kbuff_head != -2 && kbuff_tail != -1){
			/* copy the value from kernel buffer to user buffer*/
			user_buffer[trans_count] = kkb_buffer[kbuff_head];
			/* increment user buffer counter*/
			trans_count++;
			/* increment kernel head index */
			kbuff_head ++;
			kbuff_head = (kbuff_head)%KKB_BUFFER_SIZE;
		}
		/* if kernel buffer is empty , reset head and tail index*/
		if(kbuff_head == kbuff_tail){
			kbuff_head = -2;
			kbuff_tail = -1;
		}
	}
}

/* Purpose:		reset global variables 
   Input:
   Output:
*/
void reset_global (void){

	minor = -1;
	major = -1;
	EOF_flag = FALSE;
	kbuff_head = -2;/*circular q head, remove from here*/
	kbuff_tail = -1;/*circular q tail, add from here*/

	call_back = NULL;
	argP = NULL;

	user_buffer = NULL;
	user_buff_len = 0;
	trans_count = 0;
}