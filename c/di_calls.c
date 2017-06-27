/* di_calls.c: device calls */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>

struct pcb* blockQ_head;/* block queue for reading from device*/

/*
 Purpose:	device independent call that correspond to sysopen
 Input:		p - the process that tries to open a device
 			device_no - device major number of the device being
 						opened
 Output:	-1 - if device number is invalid
 			-2 - 
*/
extern int di_open(struct pcb* p, int device_no){
	/*check if the device major number is valid */
	/* no name to major device number mapping */
	if (isInRange(device_no, DEVICE_TABLE_SIZE, 0) == FALSE){
		return -1;
	}
	
	int fd_no = devtab[device_no].dvnum; /*using major to map */	
	/* if another minor type is opened already, return -1 as well */
	if (p->fd_table[fd_no].status == TRUE){
		if (p->fd_table[fd_no].major == fd_no){
			if (p->fd_table[fd_no].minor == devtab[device_no].dvminor){
				return -2; /*cannot reopen the same */
			}
			else{
				 /*cannot open other minor when a device of same major is opened*/
				return -3;
			}
		}	
	}
	/* initialize the file descriptor table entry block */
	p->fd_table[fd_no].major = devtab[device_no].dvnum;
	p->fd_table[fd_no].minor = devtab[device_no].dvminor; 	
	p->fd_table[fd_no].dev_p = &devtab[device_no]; /*LHS pointer stores an RHS address */
	p->fd_table[fd_no].ctr_data = NULL; /*just not used */
	p->fd_table[fd_no].status = TRUE; /*it's alive */
	
	/* call devopen to actually open the device */
	struct devsw *devptr;
	devptr = p->fd_table[fd_no].dev_p; 
	(devptr->dvopen)(p, fd_no);
	return fd_no;
}

/*
 Purpose:	device independent call corresponds to sysclose, closes the 
 			specified file descriptor
 Input:		p - the process that perform close action
 			device_no - the device to perform close action on
 Output:	0 - if successfully close
 			-1 - any other error
*/
extern int di_close(struct pcb* p, int device_no){
	/*check if device number is valid*/
	if (isInRange(device_no, DEVICE_TABLE_SIZE, 0) == FALSE){
		return -1;
	}
	int fd_no = devtab[device_no].dvnum; /*using major to map*/		
	/*check if there is anything opened, then we can close */
	if (p->fd_table[fd_no].status == TRUE){
		if (p->fd_table[fd_no].major == fd_no){
			if (p->fd_table[fd_no].minor == devtab[device_no].dvminor){
				/*clear it */
				p->fd_table[fd_no].status = FALSE;
				/* call dvclose to actually close the device */
				struct devsw *devptr;
				devptr = p->fd_table[fd_no].dev_p; 
				(devptr->dvclose)(p, fd_no);				
				return 0;
			}
			else{
				return -1; 
			}	
		}	
	}
	return -1; /* it was not opened, so cannot be closed */
}

/*
 Purpose:	device independent call corresponds to syswrite, which writes
 			to specified device
 Input:		p - process that performs write
 			fd - file descripter to specify device to write to 
 			buff - data source buffer to be written
 			bufflen - specifies how many bytes to write
 Output:	number of bytes wirtten -  if successful
 			-1 - on error
 			-2 - if fd is not valid
*/
extern int di_write(struct pcb* p, int fd, void* buff, int bufflen){
	/* check if fd is in range, and it is open */
	if (isInRange(fd, FD_TABLE_SIZE, 0) == FALSE){
		return -2; /*fd is out of range */
	}
	if (p->fd_table[fd].status == FALSE){
		return -1; /*device not opened */
	}
	struct devsw *devptr;
	devptr = p->fd_table[fd].dev_p; 
	return (devptr->dvwrite)(buff,bufflen);
}

/*
 Purpose:	device independent call corresponds to sysread, which performs read
 			from specified device with specified length to read
 Input:		p - process that performs read
 			fd - file descripter to specify device to read from
 			buff - data destination buffer to be read into
 			bufflen - specifies how many bytes to read 
 Output:	number of bytes read - if read is successful
 			0 - if EOF was encountered
 			-1 - other errors
*/
extern int di_read(struct pcb* p, int fd, void* buff, int bufflen, int count){
	/* check if fd is in range, and it is open */
	if (isInRange(fd, FD_TABLE_SIZE, 0) == FALSE){
		return -1; /*fd is out of range */
	}
	if (p->fd_table[fd].status == FALSE){
		return -1; /*device not opened */
	}
	/* if there's process on the blockQ already, add process on end*/
	if (blockQ_head != NULL){
		p->keyboard_fd = fd;
		p->u_buff = buff;
		p->u_buff_len = bufflen;
		blockQ_head = enqueue(blockQ_head, p, BLOCK);
		reconsReadyQ(p);
		return -9; /* will never get this value */
	}
	/* call device specific code*/
	struct devsw *devptr;
	devptr = p->fd_table[fd].dev_p;
	int result = (devptr->dvread)(buff,bufflen,(void (*)(void*, int,int))&unblock_read, (void *) p, count);

	/* check if we need to block the process */
	/* if read did not finish*/
	if (result == BLOCK_PROC){
		/* save needed parameters on pcb*/
		p->keyboard_fd = fd;
		p->u_buff = buff;
		p->u_buff_len = bufflen;
		/*add process to head of blockQ */
		reconsReadyQ(p);
		p->next = blockQ_head;
		blockQ_head = p;
		p->state = BLOCK;
		return -9;
	}
	return result; 
}

/*
 Purpose:	device independent call corresponds to sysioctl, which takes a 
 			file descriptor and executes specified control command
 Input:		p - process that performs the control command
 			fd - file descriptor of the device to read from
			command_addr - control command parameters starting address
 Output:	number of bytes read - if read is successful
 			0 - if EOF was encountered
 			-1 - other errors
*/
extern int di_ioctl(struct pcb* p, int fd, void * command_addr){
	/* check if fd is valid, and if it is keyboard, and if it is opened */
	if (isInRange(fd, FD_TABLE_SIZE, 0) == FALSE){
		return -1; /*fd is out of range */
	}
	if (p->fd_table[fd].status == FALSE){
		return -1; /*device not opened */
	}
	if (p->fd_table[fd].major != KBMON){
		return -1; /*device not a keyboard, not supported */
	}

	/* call device specific function */
	struct devsw *devptr;
	devptr = p->fd_table[fd].dev_p;	
	int result = (devptr->dvcntl)((void *)command_addr);
	return result;
}

/*
 Purpose:	checks if provided number is with in bound
 Input: 	num - the number to check
 			max - the upper bound(exclusive)
 			min - the lower bound 
 Output:	TRUE - if within bound
 			FALSE - otherwise
*/
/*from min to max-1 inclusive */
int isInRange(int num, int max, int min ){
	if (num >= max || num < min){
		return FALSE;
	}
	return TRUE;
}

/*
 Purpose:	unblock process from blockQ, add len to its return value,
 			try to read the next one on block queue
 Input:		p - the process to unblock from block queue
 			len - bytes read, wirtten to result
 			count - counter on the current reading position of user buffer
 Output:
*/
/* unblock p from block queue,
 * add len to its return value
 * try read for the next one on block queue
 */
extern void unblock_read(struct pcb* p, int len, int count){
	if (blockQ_head == NULL){
		/* nothing to unblock return */
		return;
	}
	if (blockQ_head == p){
		/* dequeue from block queue */		
		blockQ_head = blockQ_head->next;
		p->next = NULL;

		/*update p->result */
		p->create_result = len;
		/*put p back on ready queue */
		ready(p);

		/* try to fetch the next one on block queue */
		if (blockQ_head != NULL){
			di_read(blockQ_head, blockQ_head->keyboard_fd, blockQ_head->u_buff, blockQ_head->u_buff_len, count);
			/* do not care if it is blocked or not
				because the only time we call di_read here is when k_buff in kbd is empty
			 */
		}
	}	
	else {
		/*ignore the request to dequeue from middle of queue */
		return;
	}
}