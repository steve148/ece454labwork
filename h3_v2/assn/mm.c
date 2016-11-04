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
#define CHUNKSIZE            (1<<7) /* initial heap size (bytes) */

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

#define NUM_FREE_LISTS 8
#define MIN_BLOCK_SIZE 32
#define MIN_BLOCK_PWR 5

Block *avail[NUM_FREE_LISTS];

int getAvailIndex(size_t size)
{
    size_t counter = 0;
    assert(size >= 2*DSIZE);
    size--;
    while (size != 0)
    {
        size = size >> 1;
        counter++;
    }
    counter = MAX(counter - MIN_BLOCK_PWR, 0);
    return counter >= NUM_FREE_LISTS ? NUM_FREE_LISTS-1 : counter;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
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
    
    if (block != block->next)
    {
        block->prev->next = block->next;
        block->next->prev = block->prev;
        if (avail[availIndex] == block)
        {
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
        avail[availIndex] = temp;
        avail[availIndex]->next = temp;
        avail[availIndex]->prev = temp;
    }

    else
    {
        temp->next = avail[availIndex];
        temp->prev = avail[availIndex]->prev;
        temp->prev->next = temp;
        temp->next->prev = temp;
    }
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
        temp = (Block*)NEXT_BLKP(bp);
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
	if ((heap_listp = mem_sbrk(4 * 	WSIZE)) == (void *)-1)
         return -1;

    PUT(heap_listp, 0);                         // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
    heap_listp += DSIZE;

    int i;
    for (i = 0; i < NUM_FREE_LISTS; i++) {
    	avail[i] = NULL;
    }

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
                    removeFromAvail(temp);
                    return (void*)temp;
                }
                else if (size >= (asize + 2*DSIZE))
                {
                    removeFromAvail(temp);
                    int newSize = size - asize;

                    void* userPtr = (void*)temp + newSize;

                    PUT(HDRP(userPtr), PACK(asize,0));
                    PUT(FTRP(userPtr), PACK(asize,0));

                    PUT(HDRP(temp), PACK(newSize,0));
                    PUT(FTRP(temp), PACK(newSize,0));

                    appendToAvail(temp);
                    return userPtr;
                }
                temp = temp->next;
            }
            while (temp != avail[availIndex]);
        }
    }
    
    return NULL;
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
	size_t size;

	if(bp == NULL || GET_ALLOC(HDRP(bp)) == 0){
      return;
    }

	size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
	
	appendToAvail((Block*)coalesce(bp));
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
    size_t asize;
    size_t extendsize;
	char *bp;

    if (size == 0)
        return NULL;

	if (size < 512) {
        size--;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        size++;
    }

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

	if((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	extendsize = asize;
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

	return bp;
}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    if (ptr == NULL)
      return (mm_malloc(size));

    void *oldptr = ptr;
    void *newptr;
    size_t old_size = GET_SIZE(HDRP(oldptr));
	size_t padded_size;

	if (size <= DSIZE)
        padded_size = DSIZE + DSIZE;
    else
        padded_size = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

    if (padded_size <= old_size)
    {
        size_t excess = old_size - padded_size;

        if (excess >= 2*DSIZE)
        {
            newptr = (void *)oldptr+padded_size;
            PUT(HDRP(newptr), PACK(excess,0));
            PUT(FTRP(newptr), PACK(excess,0));
            appendToAvail((Block*)newptr);

            PUT(HDRP(oldptr), PACK(padded_size,1));
            PUT(FTRP(oldptr), PACK(padded_size,1));
            return oldptr;
        }

        if (excess < 2*DSIZE)
        {
            return oldptr;
        }
    }

    else
    {
        PUT(HDRP(ptr), PACK(old_size,0));
        PUT(FTRP(ptr), PACK(old_size,0));
        ptr = coalesce(ptr);

        size_t coalesced_size = GET_SIZE(HDRP(ptr));
        if (coalesced_size >= padded_size)
        {
            size_t payload_size = old_size - DSIZE;
            memmove(ptr, oldptr, payload_size);

            PUT(HDRP(ptr), PACK(coalesced_size,1));
            PUT(FTRP(ptr), PACK(coalesced_size,1));
            return ptr;
        }
        newptr = mm_malloc(size);
        if (newptr == NULL)
        {
            return NULL;
        }

        size_t payload_size = old_size - DSIZE;
        memcpy(newptr, oldptr, payload_size);

        appendToAvail((Block*)ptr);

        return newptr;
    }

    return NULL;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
  return 1;
}
