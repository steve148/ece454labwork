/*
 *	Segregated-list allocator
 *  -------------------------
 *
 *  This allocator builds on the implicit list allocator given to us by the Hallaron textbook
 *  The segregated list allocator uses a set of free lists that divide free blocks into classes.
 *  Each list holds a range of free block sizes. Whenever a malloc is called,
 *  we can save time by doing a lookup on the list based on the size asked for and give a free
 *  block of the appropriate size to the user. If no appropriate block is found in any of the
 *  free lists we will then malloc.
 *
 *  We use the policy of immediate coalescing. So, coalescing is attempted whenever mm_free()
 *  is called and whenever a size greater than the requested size for mm_realloc() is called.
 *
 *	We use the first fit method when trying to acquire free blocks for the user. In the case
 *  where we find a block that is bigger than what the request is, we attempt to split the
 * 	block. This helps improve space utilization.
 *
 *  When a realloc is done, we see if it was a shrink request and attempt to do a split. If
 *  a split can be done, we return the excess to the appropriate free list. If we can't do a
 *  split then we return the original block to the user. If the user wants to expand the block
 *  We attempt to coalesce the given block with the hopes of fulfilling the user's request.
 *  If the coalesce did not help, we call malloc and do a memmove in order to handle overlap
 *  and make sure the payload is maintained.
 *
 *	When the block is free, we use 2 pointers - stored in the first 16B of the payload to keep
 *  nodes connected in the free list (this ensures minimal overhead). We also use a header
 *  and footer in order to support forward and backward coalescing. A circular doubly-linked
 *  list is used to keep track of free blocks in each of the free lists.
 *
 *	When a block is free, we have 8 Bytes for the header, 8 Bytes for the footer, 8 bytes for
 *  the next pointer and 8 bytes for the previous pointer. Therefore our minimum block size
 *  is 32 Bytes. The pointers to next and previous are stored in the payload and are discarded
 *  once the free block is given to the user.
 *
 *  We round up smaller requests (under 512B) to powers of 2 when possible in malloc in order to 
 *  achieve better space utilization when a block is freed. We align the block to the requirements
 *  of a double word. We increase the chance that a future larger request will fit in a block we free.
 *  This has the benefit of reducing external fragmentation
 *
 *  The free list header pointers are stored globally in the array free_list_array. The array size
 *  is set to be NUM_FREE_LISTS, which we set as 8 in our implementation. As a result, the total
 *  global overhead is 8*sizeof(free_block*) = 64B on a 64-bit system.
 *
 *  Pictorial representation of an arbitrary free block
 *  ---------------------------------------------------
 *   ________  ___________  ___________  _______________  ________
 *  |size_8B_||next_ptr_8B||prev_ptr_8B||rest_of_payload||size_8B_|
 *   header    next ptr     prev ptr                      footer
 *
 *
 *	Pictorial representation of an allocated block
 *  ----------------------------------------------
 *   _____________________________________________________________
 *  |size_8B_||payload___________________________________|size_8B_|
 *   header                                               footer
 *
 *  Free list size ranges:
 *  Bucket 0:         size <= 32
 *  Bucket 1: 33   <= size <= 64
 *  Bucket 2: 65   <= size <= 128
 *  ..............
 *  Bucket 6: 1025 <= size <= 2048
 *  Bucket 7:         size >  2048
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
 * Team structure for grading
 ********************************************************/
team_t team = {
    /* Team name */
    "Sil.The.Bird",
    /* First member's full name */
    "Calvin Fernandes",
    /* First member's email address */
    "calvin.fernandes@mail.utoronto.ca",
    /* Second member's full name (leave blank if none) */
    "Everard Francis",
    /* Second member's email address (leave blank if none) */
    "everard.francis@mail.utoronto.ca"
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
 *************************************************************************/
#define WSIZE       sizeof(void *)         /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */
#define OVERHEAD    DSIZE     /* overhead of header and footer (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* custom printing functions when DEBUG mode is enabled */
#ifdef DEBUG
#define DPRINTF(fmt, ...) \
    printf(fmt, ## __VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

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

/* alignment */
#define ALIGNMENT 16
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0xf)

int mm_check();

void* heap_listp = NULL;

/* Data structures for free list management BEGIN */
typedef struct free_block
{
    struct free_block* next;
    struct free_block* prev;
} free_block;

/* Book-keeping for segregated list implementation */
#define NUM_FREE_LISTS 8	//8 free lists
#define MIN_BLOCK_SIZE 32	//32B is the minimum block size we picked for our smallest free list (<=32)
#define MIN_BLOCK_PWR 5		//2^5 - Part of the sort mechanism in our hash function, lg(MIN_BLOCK_SIZE)

free_block* free_list_array[NUM_FREE_LISTS];
/* Data structures for free list management END */

/*
 *	This function is responsible for mapping the requested
 *  size to the corresponding free list and vice versa. It performs
 *  something similar to a base-2 logarithm on the size to find the index of the correct free
 *  list. Given the typical inputs, it is basicaly an O(1) function.
 */
int hash_function(int size)
{
    int counter = 0;
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

/*
 * This function removes a free block from the free_list
 * All it needs is a pointer to the free block and it can
 * determine in exactly which free list it is due to the
 * hash_function.
 *
 * It also adjusts the free_list pointer for the corresponding
 * class size if needed - this is the case when a list has only
 * one element and the free list pointer of that class size
 * should be set to NULL
 *
 * Each list is a circular doubly link list structure
 */
void remove_from_list(free_block* block) {
    DPRINTF("REMOVE_FROM_FREE_LIST: REMOVING 0x%x\n", block);

    size_t size = GET_SIZE(HDRP(block));
    int free_list_index = hash_function(size);

    if (block == NULL)
    {
        return;
    }
    //double linked list free block removal
    if (block != block->next)
    {
        block->prev->next = block->next; // Make previous free block point to next free block
        block->next->prev = block->prev; // Make next free block point to previous free block
        if (free_list_array[free_list_index] == block)
        {
            // If we are removing the head pointer, we must set a new head pointer
            free_list_array[free_list_index] = block->next;
        }
    }
    else
    {
        free_list_array[free_list_index] = NULL;
    }
}

/*
 * This function adds a free block to the corresponding
 * free_list. It uses the hash_function to determine
 * which list the free block belongs to and puts the
 * block in that free list.
 * Each list is a circular doubly link list structure
 */
void add_to_list(free_block* temp)
{
    size_t size = GET_SIZE(HDRP(temp));
    int free_list_index = hash_function(size);

    DPRINTF("ADD_TO_LIST: ADDING 0x%x\n", temp);
    if (temp == NULL)
        return;

    if (free_list_array[free_list_index] == NULL)
    {
        // The free list is empty, this will be the first free block. Set head to point to it.
        free_list_array[free_list_index] = temp;
        free_list_array[free_list_index]->next = temp;
        free_list_array[free_list_index]->prev = temp;
    }

    else
    {
        // Insert the newly freed block into the start of the free list
        // It will become the new head
        temp->next = free_list_array[free_list_index];
        temp->prev = free_list_array[free_list_index]->prev;
        temp->prev->next = temp;
        temp->next->prev = temp;
    }
}

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 * Our free lists are initialized here as well
 * This is done to prevent errors when a call to mm_init
 * is made during a trace
 **********************************************************/
int mm_init(void)
{
    DPRINTF("MM_INIT:\n");
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                         		// alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(OVERHEAD, 1));   // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(OVERHEAD, 1));   // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    		// epilogue header
    heap_listp += DSIZE;
    // Initialize the free lists to NULL
    int i;
    for (i=0; i < NUM_FREE_LISTS; i++)
    {
        free_list_array[i] = NULL;
    }

    return 0;
}

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 * This has been modified to work free lists
 * We remove blocks from the free list that corresponds
 * to the coalescing operation. This preserves the
 * integrity of the free lists
 * 
 * Since all free blocks MUST be in the free list, we can safely
 * assume any free block found can be removed via a call to remove_from list
 **********************************************************/
void *coalesce(void *bp)
{
    DPRINTF("COALESCING\n");
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {   /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    {   /* Case 2 */
        // Remove the next free block from corresponding free_list
        free_block* temp = (free_block*)NEXT_BLKP(bp);
        remove_from_list(temp);

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc)
    {   /* Case 3 */
        // Remove the prev free block from corresponding free_list
        free_block* temp = (free_block*)PREV_BLKP(bp);
        remove_from_list(temp);

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else
    {   /* Case 4 */
        // Remove both the next and prev free blocks from corresponding free list
        free_block* temp = (free_block*)PREV_BLKP(bp);
        remove_from_list(temp);
        temp = (free_block*)NEXT_BLKP(bp);
        remove_from_list(temp);

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
    DPRINTF("EXTENDING HEAP\n");
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

    return bp;
}

/**********************************************************
 * find_fit
 * First fit method
 * Traverse the free lists looking for free blocks
 * This will attempt to look in the list corresponding
 * the size requested and start looking in bigger block lists
 * if it cannot find the requested size.
 * Splitting will be done when possible (give the user
 * a block that is more than what they asked for)
 **********************************************************/
void * find_fit(size_t asize)
{
    DPRINTF("FINDING FIT\n");

    int free_list_index = hash_function(asize);

    mm_check();
    // Traverse the free lists starting from the most appropriately sized one
    // and find the first fitting block. We search largest lists if nothing is found.
    // If there is excess space in the free block we found, we
    // place the remainder back into the free list as a smaller free block
    int i;
    for (i=free_list_index; i < NUM_FREE_LISTS; i++)
    {
        if (free_list_array[i] != NULL)
        {
            free_block* temp = free_list_array[i];

            do
            {
                int size = GET_SIZE(HDRP(temp));
                if (size >= asize && size < (asize + 2*DSIZE))
                {
                    // We found a block that can fit, but cannot be split into an excess free block
                    // This is easy to resolve, simply remove it from the free list and
                    // return it to the malloc request.
                    remove_from_list(temp);
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
                    remove_from_list(temp);
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
                    add_to_list(temp);
                    DPRINTF("AFTER FREE BLOCK SPLITTING");
                    mm_check();
                    return userPtr;
                }
                // Move onto the next pointer
                temp = temp->next;
            }
            while (temp != free_list_array[free_list_index]); // Continue until we are back where we started
        }
    }

    // Could not find a free block anywhere, let malloc handle this
    return NULL;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
    DPRINTF("PLACE - setting allocated bit\n");
    /* Get the current block size */
    size_t bsize = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 * This function frees a block and inserts it into the free_list
 * There is no additional overhead, as the space we freed up is
 * used for the entry and is invisible to the user. Remember
 * that the pointers are stored in the payload
 **********************************************************/
void mm_free(void *bp)
{
    if(bp == NULL)
    {
        return;
    }
    size_t size = GET_SIZE(HDRP(bp));
    DPRINTF("RECEIVED FREE (0x%x), size=%d\n",bp,size);

    if (GET_ALLOC(HDRP(bp)) == 0)
    {
        // Block was already freed, just return
        DPRINTF("BLOCK ALREADY FREE\n");
        return;
    }

    //mark allocated bit 0
    mm_check();
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));

    // After coalescing, we can add the new (possibly bigger) free block to the free list
    // We do not need to worry about duplicates, as the coalesce function will remove free
    // blocks from the free_list as it groups them.
    add_to_list((free_block*)coalesce(bp));

    DPRINTF("AFTER FREE:\n");
    mm_check();

}

/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes that is closest to a power
 * of 2 or that is just aligned to double word size and
 * the overhead of a block header and footer (extra book-keeping
 * is stored in the payload when the block is freed)
 * The search for free blocks is determined by find_fit
 * The decision of splitting the block, or not is determined
 * in find_fit as well.
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{

    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Bit twiddling trick: Round up to the next power of 2.
       By rounding up for smaller blocks, we increase the chance that a future larger request
       (likely a realloc) will fit in a block we free. This has the benefit of reducing external fragmentation

       Additionally, we choose a conservative number like 512 to reduce the risk of running out of
       total memory.
     */
    if (size < 512) {
        size--;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        size++;
    }

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = DSIZE + OVERHEAD;
    else
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1))/ DSIZE);

    DPRINTF("RECEIVED MALLOC: size=%d\n",asize);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        DPRINTF("FIND FIT - SERVICED MALLOC (0x%x), size=%d\n",bp,asize);
        mm_check();
        return bp;
    }

    // Don't use a minimum size, we want to target utilization performance
    // so extend the heap exactly by the request
    extendsize = asize;
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    DPRINTF("EXTENDSIZE - SERVICED MALLOC (0x%x), size=%d\n",bp,asize);
    mm_check();

    return bp;

}

/**********************************************************
 * mm_realloc
 * If size is zero then this is just a free
 * If user is shrinking then check if we can tear of excess
 * and add it to the free list otherwise just give the
 * pointer back to the user
 * If user is expanding the block then we coalesce in the
 * hopes of fulfilling the request without having to use
 * malloc. Otherwise use malloc
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    DPRINTF("RECEIVED REALLOC: (0x%x), size=%d\n",ptr,size);
    // If size == 0 then this is just free, and we return NULL.
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    // If old ptr is NULL, then this is just malloc.
    if (ptr == NULL)
        return (mm_malloc(size));

    mm_check();
    void *oldptr = ptr;
    void *newptr;

    size_t old_size = GET_SIZE(HDRP(oldptr));
    size_t padded_size;

    if (size <= DSIZE)
        padded_size = DSIZE + OVERHEAD;
    else
        padded_size = DSIZE * ((size + (OVERHEAD) + (DSIZE-1))/ DSIZE);

    // Handle shrink case
    if (padded_size <= old_size)
    {
        // CASE 1 - User wants to shrink the allocation
        //old_size = padded_size + excess
        //check if excess is >= min request size for tearing
        size_t excess = old_size - padded_size;

        //tearing possible
        if (excess >= 2*DSIZE)
        {
            //Fix the 2nd part of the block (the tear) - send this to the free list
            newptr = (void *)oldptr+padded_size;	//get a pointer to the payload of the torn block
            PUT(HDRP(newptr), PACK(excess,0));		//set the proper size for the header of the torn block - deallocate markation
            PUT(FTRP(newptr), PACK(excess,0));		//set the proper size for the footer of the torn block - deallocate markation
            add_to_list((free_block*)newptr);

            //Fix the 1st part of the block - the part we send to the user
            //The part of the relevant data is already sitting in the payload so we can return this once fixed
            PUT(HDRP(oldptr), PACK(padded_size,1));		//set the proper size for the header of the torn block - set allocated as we give to user
            PUT(FTRP(oldptr), PACK(padded_size,1));		//^ for footer
            //DPRINTF("TORN CASE HAPPENED!\n");
            return oldptr;	//give this to the user
        }

        //tearing impossible - excess data can not be torn
        if (excess < 2*DSIZE)
        {
            //DPRINTF("NO-TEAR CASE HAPPENED!\n");
            return oldptr;
        }
    }

    //CASE 2 - Handle expand case
    //User wants more data
    else
    {

        //Attempt a coalesce
        //use ptr to do the coalescing and oldptr will point to the payload

        //Mark as free so coalesce will be able to do its job
        //old_size is what the block originally was
        PUT(HDRP(ptr), PACK(old_size,0));
        PUT(FTRP(ptr), PACK(old_size,0));
        ptr = coalesce(ptr);
        mm_check();

        size_t coalesced_size = GET_SIZE(HDRP(ptr));
        if (coalesced_size >= padded_size)
        {
            //coalesce worked properly
            //remove the overhead so we can memcpy properly
            size_t payload_size = old_size-OVERHEAD;
            // We have to use memmove because of potential overlap. Memcopy does not handle overlap
            memmove(ptr, oldptr, payload_size);	//shift the payload if necessary

            //fix the allocated bits - give to user
            PUT(HDRP(ptr), PACK(coalesced_size,1));
            PUT(FTRP(ptr), PACK(coalesced_size,1));
            return ptr;
        }
        //Worst case scenario - This means an extend_heap will most likely be done, unless there is a free block available
        newptr = mm_malloc(size);
        if (newptr == NULL)
        {
            DPRINTF("Out of memory\n");
            return NULL;
        }

        //remove the overhead that is calculated using the SIZE macro and only copy what is in the payload
        //this should avoid problems
        size_t payload_size = old_size-OVERHEAD;
        memcpy(newptr, oldptr, payload_size);

        // The old location was coalesced and is now free. Add it to the free list
        add_to_list((free_block*)ptr);

        return newptr;
    }

    return NULL;
}

/**********************************************************
 * 	mm_check
 * 	Check the consistency of the memory heap
 * 	This is also responsible for printing out heap stats and
 * 	free list stats in addition to checking for errors in the
 *	allocator.
 *
 *	HOW TO USE:
 *	This is enabled in DEBUG mode by default and has full
 *  integration with functions to track every move of the dynamic
 *  memory allocator. Output can be piped to a file
 *  (size goes to over 5-6GB) so use grep to find special terms
 *	In order to enable this, please do a make clean followed by
 *	a make CFLAGS+="-DDEBUG" and then run mdriver as you wish
 *
 *  WHAT THIS DOES:
 *  In addition to printing out heap stats and free lists stats
 *  The checker will:
 *  - make sure blocks in free list are actually free
 *  - check if free blocks are within the heap in the free list
 *  - check if blocks on the heap are at least the minimum
 *    block size
 *  - check if blocks on the heap are aligned to DWORD size
 *  - check if blocks on the heap have matching headers and
 *    footers
 *  - check if blocks on the heap are overlapping
 *
 *  There is also a coalescing check but that should only be
 *  called only when mm_free() or mm_realloc() is called in
 *  order to report correctly. As a result this is commented out.
 *
 *  On normal build (no debug), this function is inlined and just returns 1
 *  so it should be optimized out
 *********************************************************/
#ifndef DEBUG
inline
#endif
int mm_check(void)
{
    int result = 1;
#ifdef DEBUG
    void* start = heap_listp;

    // This prints out a nicely formatted table of the entire heap, showing everything
    // that is allocated and unallocated, including the size of each
    DPRINTF("\n\nHEAP STATS:\n");
    while (GET_SIZE(HDRP(start)) != 0)
    {
        DPRINTF("Address: 0x%x\tSize: %d\tAllocated: %d\n",start,GET_SIZE(HDRP(start)),GET_ALLOC(HDRP(start)));
        start = NEXT_BLKP(start);
    }
    DPRINTF("Address: 0x%x\tSize: %d\tAllocated: %d [HEAP END]\n",start,GET_SIZE(HDRP(start)),GET_ALLOC(HDRP(start)));
    DPRINTF("\n");


    // Free list consistency check - check to see if all blocks in the free list are indeed free
    // Additionally, it prints out a nicely formatted table of all free list "buckets" and the free
    // blocks they contain
    int i;
    int size = -1;
    int free_status = -1;
    for (i=0; i < NUM_FREE_LISTS; i++) {
        free_block* traverse = free_list_array[i];
        DPRINTF("\nFREE LIST STATS (Range %d-%d):\n",(MIN_BLOCK_SIZE/2 << i)+1,MIN_BLOCK_SIZE/2 << (i+1));
        if (traverse != NULL)
        {
            do
            {
                size = GET_SIZE(HDRP(traverse));
                free_status = GET_ALLOC(HDRP(traverse));
                DPRINTF("Address in Free-List: 0x%x\tSize: %d\tAllocated: %d\n", traverse, size, free_status);
                //make sure blocks in free list are actually free
                if (free_status == 1)
                {
                    DPRINTF("ERROR: BLOCKS IN FREE LIST ARE ALLOCATED!\n\n\n");
                    result=0;
                    break;
                }

                //checking if pointers in the free list are within the heap
                if ((void*)traverse > mem_heap_hi() || (void*)traverse < heap_listp )
                {
                    DPRINTF("ERROR: 0x%x in the free list is not found in the heap\n\n\n", traverse);
                    result=0;
                    break;
                }

                traverse = traverse->next;
            }
            while (traverse != free_list_array[i]);  //termination condition of a circular list
        }
    }

    //Heap list consistency checker
    start = heap_listp;
    start = NEXT_BLKP(start); //skip the prologue block
    DPRINTF("\n\nBLOCK INFO STATS:\n");
    size = -1;
    while (GET_SIZE(HDRP(start)) != 0)
    {
        size = GET_SIZE(HDRP(start));
        //DPRINTF("Address: 0x%x\tSize: %d\n",start,GET_SIZE(HDRP(start)));

        //Minimum size check
        if (size < (DSIZE+OVERHEAD) || ALIGN(size) != size )
        {
            DPRINTF("\nERROR: Address: 0x%x contains a block that is less than the minimum size\n", start);
            result=0;
        }

        //DSIZE alignment check
        if (size % DSIZE)
        {
            DPRINTF("\nERROR: Address: 0x%x contains a block that is not aligned\n", start);
            result=0;
        }

        //Header footer mismatch check
        if ( GET_SIZE(HDRP(start)) != GET_SIZE(FTRP(start)) )
        {
            DPRINTF("\nERROR: Address: 0x%x has a header that does not match its footer\n", start);
            result=0;
        }

        //overlap check
        if ( FTRP(start) > HDRP(NEXT_BLKP(start)) )
        {
            DPRINTF("\nERROR: Address: 0x%x footer is greater than 0x:%x header\n", start, NEXT_BLKP(start));
            result=0;
        }

        //Coalesce check for contiguous free blocks (whether before or after)
        //This check should only be done before program termination otherwise extraneous errors will result when block splitting occurs
        //if ( (GET_ALLOC(HDRP(start)) == 0) && (GET_ALLOC(HDRP(NEXT_BLKP(start))) == 0) && start != heap_listp && NEXT_BLKP(start) != mem_heap_hi() )
        //{
        //	DPRINTF("\nERROR: Address: 0x%x has an adjacent block 0x%x that are not coalesced\n", start, NEXT_BLKP(start));
        //}

        start = NEXT_BLKP(start);
    }

#endif

    return result;
}
