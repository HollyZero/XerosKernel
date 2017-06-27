/* mem.c : memory manager */

#include <xeroskernel.h>
#include <i386.h>
#include <limits.h>


/* Declares some extern variables to use */
extern long freemem;
extern char* maxaddr;
struct memHeader *memSlot;
struct memHeader *head;		/* head pointer of the free memory block list */

/* 
	Purpose: This function initialze the memory manager by initializing the
			the free memory list. It creates 2 blocks, one before HOLE and 
			one after HOLE to manage the HOLE-problem
	Input: 	
	Output:
 */
extern void kmeminit(void){
	
	/* free memory starts here*/
	memSlot = (struct memHeader *)freemem;
	/*afterHole is used to manage HOLE problem */
	struct memHeader *afterHole;
	afterHole = (struct memHeader *)HOLEEND;
	head = memSlot;

	/*calculated size of the 2 chunks of memory */
	long sBeforeHole = HOLESTART - freemem;
	long sAfterHole = (long)maxaddr - HOLEEND;

	/* initialize first chunk of memory */
	memSlot->size = sBeforeHole;
	memSlot->prev = NULL;
	memSlot->next = afterHole;
	memSlot->sanityCheck = (char *)SANITY_VALUE;
	
	/* initialize second chunk of memory */
	afterHole->size = sAfterHole;
	afterHole->prev = memSlot;
	afterHole->next = NULL;
	afterHole->sanityCheck = (char *)SANITY_VALUE;

}


/*
 * Purpose:	This function allocates a corresponding amount of memory from the
  			available memory.Returns a pointer to the start of the most 
  			suitable memory.
  	Input:	size - the desired size of memory needed. 
  	Output:	pointer on the memory space.
 *
 */

extern void *kmalloc( int size){

	/* Compute amount of memory to set aside for this request */
	int amnt = (size)/16 + ((size%16)?1:0);
	amnt = amnt*16 + sizeof(struct memHeader);

	struct memHeader *tmpSlot = head;/*starting point to find the block*/
	struct memHeader *resultSlot = NULL;/* saves the most suitable free block*/
	
	/* Scan free memory list looking for spot */
	/* find the smallest chunk that will fit */
	long smallestDiff = LONG_MAX;/* used to store the smallest difference number*/
	while(tmpSlot != NULL ){
		long sizeDiff = tmpSlot->size - amnt;

		/*if the current diffrence between the 
			block and the required size is smaller*/
		if (sizeDiff < smallestDiff && sizeDiff >=0)
		{
			/*update the variable, save the block */
			smallestDiff = sizeDiff;
			resultSlot = tmpSlot;
		}

		tmpSlot = tmpSlot->next;
	}
	/* if no suitable block is found, 
	output error message and enter infinite loop*/
	if(resultSlot == NULL){
		kprintf("ERROR: no suitable memory found!\n");
		for(;;);
	}

	/* Determine where slot starts and overlay struct memHeader */
	/* currSlot is the new slot allocated*/
	struct memHeader *currSlot = resultSlot;
	/* move the old pointer to point after allocated block
		and update its fields*/
	/* if the resultSlot is used up by the amount required, remove block 
	from free list completely*/
	if(smallestDiff > 16){
		resultSlot = resultSlot+amnt/16;
		resultSlot->size = currSlot->size - amnt;
		resultSlot->next = currSlot->next;
		resultSlot->prev = currSlot->prev;
	}

	/* Fill in new block's memHeader fields */
	currSlot->size = amnt;
	currSlot->next = NULL;
	currSlot->prev = NULL;
	currSlot->sanityCheck = (char *)SANITY_VALUE;

	/* Adjust free memory list */
	/* slice the chunk to two parts, currSlot will be the allocated block*/
	/* and resultSlot is leftover and modified but still in the free memory list */
	
	/* case: last in the link list */
	if(resultSlot->next != NULL){
		//kprintf("inside first if: currSlot->size =%ld ,amnt = %ld\n",currSlot->size,amnt);
		resultSlot->next->prev = resultSlot;	
	}

	/* case: first in the link list */ 
	if(resultSlot->prev != NULL){
		resultSlot->prev->next = resultSlot;
	}
	/* special case: it is the head but the first part is allocated */
	/* so the head pointer has to change to point to after this allocated memory*/
	else{
		head = resultSlot;
	}

	/* Return memSlot -> dataStart */
	return currSlot->dataStart;
}

/*
 * Purpose: This function first checks to combine free chunks of memory
 *			and then free the memory block indicated by ptr
 * Input: ptr: a pointer to the memheader->dataStart
 * Output: none
 *
 */
extern void kfree(void *ptr){
	
	/* identify the start of the block*/

	struct memHeader *realStart = ptr - sizeof(struct memHeader);
	struct memHeader *realEnd = realStart + realStart->size/sizeof(struct memHeader);

//	kprintf("realstart = %ld\n realEnd = %ld\n", realStart, realEnd);
	/* check for sanity */
	if (realStart -> sanityCheck != (char *)SANITY_VALUE){
		kprintf("ERROR: memory went crazy! \n");
		for (;;);
	}

	/* traverse the free list to find if there are any adjacency free blocks */
	struct memHeader *tmp = head;

	/* Find the spot where this block is going to, and also checks if there is any adjecency */
	int adj_left = 0;
	int adj_right = 0;
	int position_found = 0;
	int change_head = 0;

	/* loop to stop once we found where this block is suppose to go in the link list */
	/* I think the list must keep order in order for the adjencency merge to work */
	while (tmp != NULL && position_found != 1){
		struct memHeader *tmp_end = tmp + tmp->size / sizeof(struct memHeader);
		position_found = 0; /* reset position_found */
//		kprintf ("in while loop tmp_end = %ld\n", tmp_end);
		/* find the right position, so this newly released block is between tmp_end and tmp->next */
		if (tmp_end <= realStart && (tmp->next != NULL && realEnd <= tmp->next)){
			position_found = 1;
			/* there is an adjacent block on the left */
			if (tmp_end == realStart){
				adj_left = 1;
			}
			if (tmp-> next != NULL && realEnd == tmp->next){
				adj_right = 1;
			}
		}
		/* special case: tmp is head and realEnd <= tmp */
		if ( realEnd <= tmp){
			position_found = 1;
			change_head = 1;
			if (realEnd == tmp){
				adj_right = 1;
			}
		}
		/* special case: tmp is tail and realStart >= tmp */
		if (realStart >= tmp){
			position_found = 1;
			if (realStart == tmp){
				adj_left = 1;
			}	
		}
		/* if position is not found, move to the next one in the link list to check */
		if (position_found != 1){
			tmp = tmp-> next;
		}
	}
//	kprintf("right = %d and left = %d\n", adj_right, adj_left);
	/* merge left free block or right free block if there are any */
	/* merge right free block first */
	if (adj_right == 1){
		/* find the right pointer to the start of right side block */
		struct memHeader *currMem;
		if (change_head == 1){
			currMem = tmp;
		}
		else {
			currMem = tmp->next;
		}
		/* use the memory header already created for the newly released block */

		realStart->size = realStart->size + currMem->size;
		/* update next and prev */
		realStart->next = currMem->next;
		realStart->prev = currMem->prev;
		if (currMem->prev != NULL){
			currMem->prev->next = realStart;
		}
		if (currMem->next != NULL){
			currMem->next->prev = realStart;
		}

		/*special case: merged right and moved head */
		if (change_head == 1){
			head = realStart;
		}
		
	}
	/* merge left free block second, it also takes care of the situation where there is free right block as well*/
	if (adj_left == 1){
		/* use the memory header of the current free block pointed to */
		tmp->size = tmp->size + realStart->size;

	}
	/* none on left and none on right, just insert this block in the right place */
	if (adj_right == 0 && adj_left == 0){
		/*special case: before head, need to move head */
		if (change_head == 1){
			realStart->next = tmp;
			tmp->prev = realStart;
			head = realStart;
		}
		else{
			realStart->next = tmp->next;
			realStart->prev = tmp;
			if (tmp->next != NULL){
				tmp->next->prev = realStart;
			}
		tmp->next = realStart;
		}
	}

}

/*	Purpose: Print the current free memory list for debugging purposes.
	Input:
	Output:
*/
extern void printMemList (void){

	struct memHeader *temp = head;
	while(temp != NULL){
		kprintf("Address of current block is: %ld, Size = %ld\n ", (long)temp,temp->size);
		temp = temp->next;
	}
	kprintf("\n");
}
