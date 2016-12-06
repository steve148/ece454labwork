/*****************************************************************************
 * life.c
 * The original sequential implementation resides here.
 * Do not modify this file, but you are encouraged to borrow from it
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <pthread.h>

/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])

typedef struct {
    char * inboard;
    char * outboard;
    int start;
    int end;
    int LDA;
    int nrows;
    int ncols;
} thread_args;

void* parallel_game_of_life (void *args) {
    int i, j;

    thread_args tinfo = *((thread_args *) args);
    free(args);

    int start = tinfo.start;
    int end = tinfo.end;
    int nrows = tinfo.nrows;
    int ncols = tinfo.ncols;

    /* HINT: you'll be parallelizing these loop(s) by doing a
       geometric decomposition of the output */
    for (i = start; i < end; i++)
    {
        const int inorth = mod (i-1, nrows);
        const int isouth = mod (i+1, nrows);
        for (j = 0; j < ncols; j++)
        {
	    const int jwest = mod (j-1, ncols);
	    const int jeast = mod (j+1, ncols);
    
	    __transaction_atomic {
		const char neighbor_count = 
		    BOARD (inboard, inorth, jwest) + 
		    BOARD (inboard, inorth, j) + 
		    BOARD (inboard, inorth, jeast) + 
		    BOARD (inboard, i, jwest) +
		    BOARD (inboard, i, jeast) + 
		    BOARD (inboard, isouth, jwest) +
		    BOARD (inboard, isouth, j) + 
		    BOARD (inboard, isouth, jeast);
    
		BOARD(outboard, i, j) = alivep (neighbor_count, BOARD (inboard, i, j));
	    }
        }
    }

    return NULL;    
}

    char*
sequential_game_of_life (char* outboard, 
        char* inboard,
        const int nrows,
        const int ncols,
        const int gens_max)
{
    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */

    // gens_max is the maximum number of generations

    const int LDA = nrows;
    int curgen, i, j;

    int numThreads = 4;
    pthread_t tid[numThreads];
    thread_args *tinfo = malloc(numThreads*sizeof(ThreadInfo));

    int rowStart = 0;
    int rowStride = nrows / numThreads;
    int rowEnd = rowStride;

    for (i = 0; i < numThreads; i++) 
    {
	tinfo[i].start = rowStart;
	tinfo[i].end = rowEnd;

	tinfo[i].LDA = nrows;
	tinfo[i].nrows = nrows;
	tinfo[i].ncols = ncols;
	
	rowStart = rowEnd;
	rowEnd += rowStride;
    }

    for (curgen = 0; curgen < gens_max; curgen++)
    {
	for (i = 0; i < numThreads; i++)
	{
	    err = pthread_create(&tid[i], NULL, &parallel_game_of_life, tinfo[i]);
	    if (err != 0) {
		printf("\nERROR CREATING THREAD: %d\n", err);
	    }
	}

	for (i = 0; i < numThreads; i++) 
    	{
	    pthread_join(&tid[i], NULL);    
    	}

	SWAP_BOARDS( outboard, inboard );
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}


