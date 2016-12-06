/*****************************************************************************
 * life.c
 * The original sequential implementation resides here.
 * Do not modify this file, but you are encouraged to borrow from it
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
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
    char* inboard;
    char* outboard;
    int start;
    int end;
    int LDA;
    int nrows;
    int ncols;
} thread_args;

pthread_t tid[4];

void* parallel_game_of_life (void *args) {
    int i, j;

    thread_args tinfo = *((thread_args *) args);

    int start = tinfo.start;
    int end = tinfo.end;
    int nrows = tinfo.nrows;
    int ncols = tinfo.ncols;
    int LDA = tinfo.LDA;

    char* inboard = tinfo.inboard;
    char* outboard = tinfo.outboard;
    /* HINT: you'll be parallelizing these loop(s) by doing a
       geometric decomposition of the output */
    for (j = 0; j < ncols; j++)
    {
        const int jwest = j ? j - 1 : ncols - 1;//mod (j-1, ncols);
        const int jeast = (j != ncols - 1) ? j + 1 : 0;//mod (j+1, ncols); 
        for (i = start; i < end; i++)
        {
            const int inorth = i ? i - 1 : nrows - 1;//mod (i-1, nrows);
            const int isouth = (i != nrows - 1) ? i + 1 : 0;//mod (i+1, nrows);
    
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
    int curgen, i, err;

    int numThreads = 4;
    thread_args* tinfo[4];

    int rowStart = 0;
    int rowStride = nrows >> 2;
    int rowEnd = rowStride;

    for (i = 0; i < numThreads; i++) 
    {
        tinfo[i] = (thread_args*) malloc(sizeof(thread_args));
	    tinfo[i]->start = rowStart;
	    tinfo[i]->end = rowEnd;

	    tinfo[i]->LDA = LDA;
	    tinfo[i]->nrows = nrows;
	    tinfo[i]->ncols = ncols;

	    rowStart = rowEnd;
	    rowEnd += rowStride;
    }

    for (curgen = 0; curgen < gens_max; curgen++)
    {
	    for (i = 0; i < numThreads; i++)
	    {
            tinfo[i]->inboard = inboard;
            tinfo[i]->outboard = outboard;
	        err = pthread_create(&tid[i], NULL, &parallel_game_of_life, (void*) tinfo[i]);
	        if (err != 0) {
	    	    printf("\nERROR CREATING THREAD: %d\n", err);
	        }
	    }

	    for (i = 0; i < numThreads; i++) 
        {
	        pthread_join(tid[i], NULL);    
        }

	    SWAP_BOARDS( outboard, inboard );
    }

    for (i = 0; i < numThreads; i++) 
    {
        free(tinfo[i]);
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}


