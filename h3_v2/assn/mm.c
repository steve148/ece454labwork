/*
 * This implementation replicates the implicit list implementation
 * provided in the textbook
 * "Computer Systems - A Programmer's Perspective"
 * Blocks are never coalesced or reused.
 * Realloc is implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Team Awesome",
    /* First member's full name */
    "Lennox Stevenson",
    /* First member's email address */
    "lennox.stevenson@mail.utoronto.ca",
    /* Second member's full name (leave blank if none) */
    "Rahul Chandan",
    /* Second member's email address (leave blank if none) */
    "rahul.chandan@mail.utoronto.ca"
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       		 sizeof(void *)                  /* word size (bytes) */
#define DSIZE       		 (2 * WSIZE)                     /* doubleword size (bytes) */
#define POOL_ORDER  		 20
#define MIN_ORDER   		 1
#define NUM_BUDDY_ALLOCATORS (POOL_ORDER - MIN_ORDER)
#define CHUNKSIZE            (DSIZE << (NUM_BUDDY_ALLOCATORS+1)) /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))
#define MIN

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

void* heap_listp = NULL;

/**********************************************************
 * structures for buddy allocator
 * 
 *
 **********************************************************/

typedef struct Blocks {
    struct Blocks *next;
    struct Blocks *prev;
} Block;   

unsigned long base_addr;

/* Book-keeping for segregated list implementation */
#define NUM_FREE_LISTS 8	//8 free lists
#define MIN_BLOCK_SIZE 32	//32B is the minimum block size we picked for our smallest free list (<=32)
#define MIN_BLOCK_PWR 5		//2^5 - Part of the sort mechanism in our hash function, lg(MIN_BLOCK_SIZE)

Block *avail[NUM_FREE_LISTS];

int getAvailIndex(size_t size)
{
    size_t counter = 0;
    assert(size >= 2*DSIZE);
    size--;
    while (size != 0)
    {
        size = size >> 1;	//use bit shifts to determine how long we take to get to MSB
        counter++;			//record MSb length
    }
    counter = MAX(counter - MIN_BLOCK_PWR, 0);
    return counter >= NUM_FREE_LISTS ? NUM_FREE_LISTS-1 : counter;
}

/**********************************************************
 * functions for buddy allocator
 *
 *
 **********************************************************/

Block *findBlock(void *bp)
{
    size_t blockOrder = getAvailIndex(GET_SIZE(HDRP(bp)));
    Block *outBlock = NULL;
    Block *tmpBlock = avail[blockOrder].head;
    while(tmpBlock != NULL)
    {
        if (tmpBlock->bp == bp)
        {
            outBlock = tmpBlock;
            break;
        }
        tmpBlock = tmpBlock->next;
    }
    return outBlock;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
	/* Get the current block size */
	size_t bsize = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(bsize, 1));
	PUT(FTRP(bp), PACK(bsize, 1));	
}

void removeFromAvail(Block *block)
{
    size_t size = GET_SIZE(HDRP(block));
    int availIndex = getAvailIndex(size);

    if (block == NULL)
    {
        return;
    }
    //double linked list free block removal
    if (block != block->next)
    {
        block->prev->next = block->next; // Make previous free block point to next free block
        block->next->prev = block->prev; // Make next free block point to previous free block
        if (avail[availIndex] == block)
        {
            // If we are removing the head pointer, we must set a new head pointer
            avail[availIndex] = block->next;
        }
    }
    else
    {
        avail[availIndex] = NULL;
    }
}

void appendToAvail(Block *temp)
{   
    size_t size = GET_SIZE(HDRP(temp));
    int availIndex = getAvailIndex(size);

    if (temp == NULL)
        return;

    if (avail[availIndex] == NULL)
    {
        // The free list is empty, this will be the first free block. Set head to point to it.
        avail[availIndex] = temp;
        avail[availIndex]->next = temp;
        avail[availIndex]->prev = temp;
    }

    else
    {
        // Insert the newly freed block into the start of the free list
        // It will become the new head
        temp->next = avail[availIndex];
        temp->prev = avail[availIndex]->prev;
        temp->prev->next = temp;
        temp->next->prev = temp;
    }
}
 
Block * splitToSize(Block *block, size_t currOrder, size_t desiredOrder)
{
    void *buddyBlock;
    
	printf("c\n"); 
	// block marked as allocated
    // store desired block size

	printf("%d\n", block->bp);

    PUT(HDRP(block->bp), PACK((size_t) DSIZE<<desiredOrder, 1));

	printf("triplefuck\n");
    PUT(FTRP(block->bp), PACK((size_t) DSIZE<<desiredOrder, 1));

	printf("c1\n");
	
    // remove block from avail[currOrder]
    removeFromAvail(currOrder, block);

	printf("d\n");

    while (currOrder > desiredOrder)
    {
		printf("e\n");
        --currOrder;
        buddyBlock = block->bp + (DSIZE << currOrder);
        
		// buddyBlock marked as free
        // store buddyBlock size
        PUT(HDRP(buddyBlock), PACK((int) DSIZE<<currOrder, 0));
        PUT(FTRP(buddyBlock), PACK((int) DSIZE<<currOrder, 0));
        
		// append buddy block to avail[currOrder]
        appendToAvail(currOrder, buddyBlock);
    }
    return block;
}
 
void * find_buddy(Block *block, unsigned long order)
{
    unsigned long _block;
    unsigned long _buddy;
    
    assert((unsigned long) block >= base_addr);
    
    _block = (unsigned long) block - base_addr;
    _buddy = _block ^ (DSIZE << order);
    
	return (void *)(_buddy + base_addr);
}

void mergeBuddies(Block *block, size_t order)
{
    while (order < POOL_ORDER)
    {
        Block *buddy = find_buddy(block, order);
        void *buddy_bp = buddy->bp;
        if (GET_ALLOC(HDRP(buddy_bp)))
        {
            break;
        }
        if (order != getAvailIndex(GET_SIZE(HDRP(buddy_bp))))
        {
            break;
        }
        // can merge
        removeFromAvail(order, buddy_bp);
        if (buddy < block) block = buddy;
        ++order;
        PUT(FTRP(block->bp), PACK(DSIZE<<order, 0));
        PUT(HDRP(block->bp), PACK(DSIZE<<order, 0));
    }
    
    PUT(FTRP(block->bp), PACK(DSIZE<<order, 0));
    PUT(HDRP(block->bp), PACK(DSIZE<<order, 0));
    appendToAvail(order, block->bp);
}

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {       /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        Block* temp = (Block*)NEXT_BLKP(bp);
        removeFromAvail(temp);
        
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        Block* temp = (Block*)PREV_BLKP(bp);
        removeFromAvail(temp);
    
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        Block* temp = (Block*)PREV_BLKP(bp);
        removeFromAvail(temp);
        Block* temp = (Block*)NEXT_BLKP(bp);
        removeFromAvail(temp);
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return bp;
}

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
int mm_init(void) {
	void *_bp;

	if ((heap_listp = mem_sbrk(4 * 	WSIZE)) == (void *)-1)
         return -1;

	printf("here");

    PUT(heap_listp, 0);                         // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
    heap_listp += DSIZE;

	printf("here2");

    int i;
    for (i = MIN_ORDER; i < POOL_ORDER; i++) {
    	avail[i] = NULL;
    }
     
    base_addr = (unsigned long) heap_listp;

    return 0;
 }


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    void *bp;
    
    int availIndex = getAvailIndex(asize);
    int i;
    for (i = availIndex; i < NUM_FREE_LISTS; i++)
    {
        if (avail[i] != NULL)
        {
            Block* temp = avail[i];

            do
            {
                int size = GET_SIZE(HDRP(temp));
                if (size >= asize && size < (asize + 2*DSIZE))
                {
                    // We found a block that can fit, but cannot be split into an excess free block
                    // This is easy to resolve, simply remove it from the free list and
                    // return it to the malloc request.
                    removeFromAvail(temp);
                    return (void*)temp;
                }
                else if (size >= (asize + 2*DSIZE))
                {
                    // The block found has excess size and we can split the excess
                    // into a free block
                    // Example: Requested size = 4
                    //          Found free block size = 10
                    // [H1][ ][ ][ ][ ][ ][ ][ ][ ][F1]
                    // newSize = 6
                    removeFromAvail(temp);
                    int newSize = size - asize;

                    // Point to the ending portion of the free block, so that
                    // we can assign this portion to the user
                    void* userPtr = (void*)temp + newSize;

                    // Set the header/footer of the user required block with their requested size
                    // Example (cont): [H1][ ][ ][ ][ ][ ][H2][ ][ ][F2]
                    PUT(HDRP(userPtr), PACK(asize,0));
                    PUT(FTRP(userPtr), PACK(asize,0));

                    // Adjust the size of the free block
                    // Example (cont): [H1][ ][ ][ ][ ][F1][H2][ ][ ][F2]
                    PUT(HDRP(temp), PACK(newSize,0));
                    PUT(FTRP(temp), PACK(newSize,0));

                    // Return the excess part to the corresponding free list
                    addToAvail(temp);
                    return userPtr;
                }
                // Move onto the next pointer
                temp = temp->next;
            }
            while (temp != avail[availIndex]); // Continue until we are back where we started
        }
    }
    
    // could not find a fit
    return NULL;
}

/**********************************************************
 * dequeue
 * Remove the marked block from the free list
 **********************************************************/
void dequeue(int block_level) {
	Block *block_to_remove = avail[block_level].head;
	avail[block_level].head = block_to_remove->next;
	return;
}

/**********************************************************
 * find_block_level
 * Find the block level for the level that fits the asize 
 **********************************************************/
size_t find_block_level(size_t asize) {
	int cntr = 1;
	while (asize > (size_t)(DSIZE << cntr)) {
		cntr = cntr << 1;
	}
	return (size_t)cntr;
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
	Block *block;
	size_t block_level;
	size_t size = GET_SIZE(HDRP(bp));

	if(bp == NULL){
      return;
    }
	
	block_level = getAvailIndex(GET_SIZE(HDRP(bp)));
	block = appendToAvail(block_level, bp);
    
	PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    coalesce(block);
}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{
    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
	size_t block_level; // current block size
    Block *block;
	void *_bp;
	int i;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

	int power = 2;
	while (asize >= power)
		power <<= 1;
	asize = (size_t)power;

	printf("asize %d\n", (int)asize);


	block_level = find_block_level(asize);// find block level
	printf("block_level %d\n", block_level);
	// if block level n has free block
	// get free block, dequeue, place, return
	if (avail[block_level].head != NULL) {
		block = avail[block_level].head;
		_bp = block->bp;
		dequeue(block_level);
		place(_bp, asize);
		printf("asize after %d\n", (int)GET_SIZE(HDRP(_bp)));
		return _bp;
	}

	// find if block level m > n has free block
	for (i = block_level; i < NUM_BUDDY_ALLOCATORS; i++) {

		// if block level m has available block, split and use
		printf("head at %d exists %d\n", i, avail[i].head != NULL);
		if (avail[i].head != NULL) {
			printf("a\n");
			block = splitToSize(avail[block_level].head, i, block_level);
            printf("b\n");
			_bp = block->bp;
			dequeue(block_level);
			place(_bp, asize);
			return _bp;
		}
	}

	printf("fuck\n");
	
	// if no blocks above are free, need to add new level
	// to block allocator

	// extend heap
	// add new tier
	// split that tier
	// set the block
		
    /* Search the free list for a fit 
    if ((block = find_fit(asize)) != NULL) {
		_bp = block-> bp;
		place(_bp, asize);
		dequeue(block);
        return _bp;
    }
    // No fit found. Get more memory and place the block 
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
	*/

}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
  return 1;
}
