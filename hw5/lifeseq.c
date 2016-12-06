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

#define BOARD( __board, __i, __j )  (__board[(__i) + nrows*(__j)])
#define NUM_THREADS         8
#define LOG2_NUM_THREADS    3

// Check for if BOARD element is alive
#define IS_ALIVE(var) ((var) & (1<<(4)))

typedef struct {
    char* inboard;
    char* outboard;
    int start;
    int end;
    int nrows;
    int ncols;
} thread_args;

// TODO: Parallelize this later, using TM
// or possibly in-line
void init_bitmap (char* board, const int nrows, const int ncols) {
    int i, j;

    for (i = 0; i < nrows; i++)
    {
        for (j = 0; j < ncols; j++)
        {
            if (BOARD(board, i, j) == (char) 1)
            {
                board[i] = board[i] << 4;
            }
            if (IS_ALIVE(BOARD(board, i, j)))
            {
                const int j_nrows = j * nrows;

                const int inorth = i ? i - 1 : nrows - 1;
                const int isouth = (i != nrows - 1) ? i + 1 : 0;
                const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
                const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

                INCREMENT_AT_COORD(board, inorth, jwest);
                INCREMENT_AT_COORD(board, inorth, j_nrows);
                INCREMENT_AT_COORD(board, inorth, jeast);
                INCREMENT_AT_COORD(board, isouth, jwest);
                INCREMENT_AT_COORD(board, isouth, j_nrows);
                INCREMENT_AT_COORD(board, isouth, jeast);
                INCREMENT_AT_COORD(board, i, jwest);
                INCREMENT_AT_COORD(board, i, jeast);
            }
        }
    }
}

void* parallel_game_of_life (void *args) {
    int i, j;

    thread_args tinfo = *((thread_args *) args);

    int start = tinfo.start;
    int end = tinfo.end;
    int nrows = tinfo.nrows;
    int ncols = tinfo.ncols;

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

    int curgen, i, err;

    int numThreads = NUM_THREADS;
    pthread_t tid[NUM_THREADS];
    thread_args* tinfo = (thread_args *)malloc(numThreads*sizeof(thread_args));

    int rowStart = 0;
    int rowStride = nrows >> LOG2_NUM_THREADS;
    int rowEnd = rowStride;

    for (i = 0; i < numThreads; i++) 
    {
	    tinfo[i].start = rowStart;
	    tinfo[i].end = rowEnd;

	    tinfo[i].nrows = nrows;
	    tinfo[i].ncols = ncols;

	    rowStart = rowEnd;
	    rowEnd += rowStride;
    }

    init_bitmap(inboard, ncols, nrows);
    return inboard;

    for (curgen = 0; curgen < gens_max; curgen++)
    {
	    for (i = 0; i < numThreads; i++)
	    {
            tinfo[i].inboard = inboard;
            tinfo[i].outboard = outboard;
	        err = pthread_create(&tid[i], NULL, &parallel_game_of_life, (void*) &tinfo[i]);
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

    free(tinfo);

    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}


