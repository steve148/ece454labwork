/*****************************************************************************
 * life.c
 * The original sequential implementation resides here.
 * Do not modify this file, but you are encouraged to borrow from it
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

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
#define MY_BOARD( __board, __i, __j )  (__board[(__i) + (__j)])
#define NUM_THREADS         8
#define LOG2_NUM_THREADS    3

typedef struct {
    char* inboard;
    char* outboard;
    int start;
    int end;
    int nrows;
    int ncols;
} thread_args;

pthread_mutex_t lock[1024];

// TODO: Parallelize this later, using TM
// or possibly in-line
void init_bitmap (char* board, const int nrows, const int ncols) {
    int i, j;

    for (i = 0; i < ncols*nrows; i++)
    {
        if (board[i] == (char) 0x01)
        {
            board[i] = board[i] << 4;
        }
    }

    for (i = 0; i < nrows; i++)
    {
        for (j = 0; j < ncols; j++)
        {
            if (IS_ALIVE(BOARD(board, i, j)))
            {
                // const int j_nrows = j * nrows;

                const int inorth = mod(i-1, nrows); //i ? i - 1 : nrows - 1;
                const int isouth = mod(i+1, nrows); //(i != nrows - 1) ? i + 1 : 0;
                const int jwest = mod(j-1, ncols); //j ? j_nrows - nrows : (ncols - 1) * nrows;
                const int jeast = mod(j+1, ncols); //(j != ncols - 1) ? j_nrows + nrows : 0;

                pthread_mutex_lock(&lock[0]);
                INCREMENT_AT_COORD(board, inorth, jwest);
                INCREMENT_AT_COORD(board, inorth, j);//j_nrows);
                INCREMENT_AT_COORD(board, inorth, jeast);
                INCREMENT_AT_COORD(board, isouth, jwest);
                INCREMENT_AT_COORD(board, isouth, j);//j_nrows);
                INCREMENT_AT_COORD(board, isouth, jeast);
                INCREMENT_AT_COORD(board, i, jwest);
                INCREMENT_AT_COORD(board, i, jeast);
                pthread_mutex_unlock(&lock[0]);
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
        // const int j_nrows = j*nrows;
        // const int jwest = j ? j_nrows - nrows: (ncols - 1) * nrows;
        // const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

        for (i = start; i < end; i++)
        {
            // char cell = MY_BOARD(inboard,i,j_nrows);
            char cell = BOARD(inboard,i,j);
    
            if (!IS_ALIVE(cell))
            {
                if (cell == (char) 0x3) {
                    // const int inorth = i ? i - 1 : nrows - 1;//mod (i-1, nrows);
                    // const int isouth = (i != nrows - 1) ? i + 1 : 0;//mod (i+1, nrows);

                    SET_ALIVE(BOARD(outboard, i, j));

                    // const int j_nrows = j * nrows;

                    const int inorth = mod(i-1, nrows); //i ? i - 1 : nrows - 1;
                    const int isouth = mod(i+1, nrows); //(i != nrows - 1) ? i + 1 : 0;
                    const int jwest = mod(j-1, ncols); //j ? j_nrows - nrows : (ncols - 1) * nrows;
                    const int jeast = mod(j+1, ncols); //(j != ncols - 1) ? j_nrows + nrows : 0;

                    pthread_mutex_lock(&lock[0]);
                    INCREMENT_AT_COORD(outboard, inorth, jwest);
                    INCREMENT_AT_COORD(outboard, inorth, j);//j_nrows);
                    INCREMENT_AT_COORD(outboard, inorth, jeast);
                    INCREMENT_AT_COORD(outboard, isouth, jwest);
                    INCREMENT_AT_COORD(outboard, isouth, j);//j_nrows);
                    INCREMENT_AT_COORD(outboard, isouth, jeast);
                    INCREMENT_AT_COORD(outboard, i, jwest);
                    INCREMENT_AT_COORD(outboard, i, jeast);
                    pthread_mutex_unlock(&lock[0]);
                    
                    //assert(i + jeast == i + mod(j + 1, ncols) * nrows);
                    //assert(i + jwest == i + mod(j - 1, ncols) * nrows);

                    //pthread_mutex_lock(&lock[0]);
                    //INCREMENT_AT_COORD(outboard, inorth, jwest);
                    //INCREMENT_AT_COORD(outboard, inorth, j_nrows);
                    //INCREMENT_AT_COORD(outboard, inorth, jeast);

                    //INCREMENT_AT_COORD(outboard, isouth, jwest);
                    //INCREMENT_AT_COORD(outboard, isouth, j_nrows);
                    //INCREMENT_AT_COORD(outboard, isouth, jeast);

                    //INCREMENT_AT_COORD(outboard, i, jwest);
                    //INCREMENT_AT_COORD(outboard, i, jeast);
                    //pthread_mutex_unlock(&lock[0]);
                }
            }
            else
            {
                if (cell <= (char) 0x11 || cell >= (char) 0x14) {
                    // const int inorth = i ? i - 1 : nrows - 1;//mod (i-1, nrows);
                    // const int isouth = (i != nrows - 1) ? i + 1 : 0;//mod (i+1, nrows);

                    SET_DEAD(BOARD(outboard, i, j));

                    // const int j_nrows = j * nrows;

                    const int inorth = mod(i-1, nrows); //i ? i - 1 : nrows - 1;
                    const int isouth = mod(i+1, nrows); //(i != nrows - 1) ? i + 1 : 0;
                    const int jwest = mod(j-1, ncols); //j ? j_nrows - nrows : (ncols - 1) * nrows;
                    const int jeast = mod(j+1, ncols); //(j != ncols - 1) ? j_nrows + nrows : 0;

                    pthread_mutex_lock(&lock[0]);
                    DECREMENT_AT_COORD(outboard, inorth, jwest);
                    DECREMENT_AT_COORD(outboard, inorth, j);//j_nrows);
                    DECREMENT_AT_COORD(outboard, inorth, jeast);
                    DECREMENT_AT_COORD(outboard, isouth, jwest);
                    DECREMENT_AT_COORD(outboard, isouth, j);//j_nrows);
                    DECREMENT_AT_COORD(outboard, isouth, jeast);
                    DECREMENT_AT_COORD(outboard, i, jwest);
                    DECREMENT_AT_COORD(outboard, i, jeast);
                    pthread_mutex_unlock(&lock[0]);
                    
                    // pthread_mutex_lock(&lock[0]);
                    // DECREMENT_AT_COORD(outboard, inorth, jwest);
                    // DECREMENT_AT_COORD(outboard, inorth, j_nrows);
                    // DECREMENT_AT_COORD(outboard, inorth, jeast);

                    // DECREMENT_AT_COORD(outboard, isouth, jwest);
                    // DECREMENT_AT_COORD(outboard, isouth, j_nrows);
                    // DECREMENT_AT_COORD(outboard, isouth, jeast);

                    // DECREMENT_AT_COORD(outboard, i, jwest);
                    // DECREMENT_AT_COORD(outboard, i, jeast);
                    // pthread_mutex_unlock(&lock[0]);
                }
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

    int curgen, i, err;

    int numThreads = NUM_THREADS;
    pthread_t tid[NUM_THREADS];
    thread_args* tinfo = (thread_args *)malloc(numThreads*sizeof(thread_args));

    init_bitmap(inboard, ncols, nrows);

    for (i = 0; i < 1024; i++)
      pthread_mutex_init(&lock[i],NULL);

    int rowStart = 0;
    int rowStride = nrows >> LOG2_NUM_THREADS;
    int rowEnd = rowStride - 1;

    for (i = 0; i < numThreads; i++) 
    {
	    tinfo[i].start = rowStart;
	    tinfo[i].end = rowEnd;

	    tinfo[i].nrows = nrows;
	    tinfo[i].ncols = ncols;

	    rowStart = rowEnd;
	    rowEnd += rowStride;
    }

    for (curgen = 0; curgen < gens_max; curgen++)
    {
        memmove (outboard, inboard, nrows * ncols * sizeof (char));
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
    
    for (i = 0; i < ncols*nrows; i++)
    {
        inboard[i] >>= 4;
    }

    return inboard;
}


