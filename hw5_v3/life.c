/*****************************************************************************
 * life.c
 * Lennox Stevenson 999585667
 * Rahul C #########
 * 
 * Parallelized and optimized implementation of the game of life resides here.
 * We optimized the Game of Life by using threads and a change in the way 
 * that the algorithm checks for its neighbour's state.
 * 
 * This biggest change was the way we check for a neighbour's state. In the 
 * original implementation we checked all 8 neighbour's states, per cell, per 
 * generation, and using that information along with the cell's state, we were 
 * able to determine if we should stay alive or die. 
 * 
 * In the new implementation, we store a cell's state along with all of it's 
 * neighbour's states in a single byte. The motivation behind this is that we 
 * can just look at a cell's own value to determine if it should be alive, 
 * where we previously checked all neighbour's value as well as a cell's own value. 
 * 
 * The overhead needed to maintain the neighbour counts within each byte is 
 * the same as the original implementation only if a cell changes state, as 
 * we have to inform all of our neighbours of our change. However, in the 
 * Game of Life, cells on average don't change. Therefore we no longer pay the 
 * overhead of counting neighbouring cells when no change has occurred. This 
 * implementation gave us a our biggest performance boost, making our final 
 * implementation 13x faster than the original.
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
game_of_life (char* outboard, 
    char* inboard,
    const int nrows,
    const int ncols,
    const int gens_max)
{
  return threaded_game_of_life (outboard, inboard, nrows, ncols, gens_max);
}

/*****************************************************************************
 * Arguements that we use to pass parameters to gol worker threads
 ****************************************************************************/
typedef struct thread_args {
  char * inboard;
  char * outboard;
  int nrows;
  int ncols;
  int start;
  int end;
} thread_args;

/*****************************************************************************
 * threaded_gol
 * Threaded version of the game of life
 ****************************************************************************/
  char*
threaded_game_of_life (char* outboard, 
    char* inboard,
    const int nrows,
    const int ncols,
    const int gens_max)
{
  int curgen, i, j;

  // Set up thread variables
  int num_threads = 8;
  pthread_t tid[num_threads];
  thread_args *tinfo = malloc(num_threads * sizeof(thread_args));

  // Set up thread args
  int start_row = 0;
  int chunk_size = nrows/num_threads;
  int end_row = chunk_size;
  for (i=0; i<num_threads; i++){
    // Set start and end thread args
    tinfo[i].start = start_row+1;
    tinfo[i].end = end_row-1;

    // Update for next start and end rows
    start_row = end_row;
    end_row = end_row + chunk_size;
  }

  // Iterate through generations 
  for (curgen = 0; curgen < gens_max; curgen++) {
    // Set outboard to be inboard just in case
    memmove (outboard, inboard, nrows * ncols * sizeof (char));

    // Before starting thread jobs, do first and last line of each chunk so
    // so the threads do not overlap while writing to outboard.
    for (i = 0; i < num_threads; i++) {
      game_of_life_single_row(tinfo[i].start - 1, ncols, nrows, inboard, outboard);
      game_of_life_single_row(tinfo[i].end, ncols, nrows, inboard, outboard);
    }

    // Start threads
    for (i = 0; i < num_threads; i++) {
      tinfo[i].nrows = nrows;
      tinfo[i].ncols = ncols;
      tinfo[i].inboard = inboard;
      tinfo[i].outboard = outboard;
      pthread_create(&tid[i], NULL, game_of_life_thread, (void*) &tinfo[i]);
    }

    // Wait for threads to finish
    for (i = 0; i < num_threads; i++) {
      pthread_join(tid[i],NULL);
    }

    // Swap the boards to update the inboard
    SWAP_BOARDS(outboard, inboard);
  }

  // Format output so that it only shows dead or alive
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      BOARD(inboard,i,j) = BOARD(inboard,i,j) >> 4;
    }
  }

  free(tinfo);
  return inboard;
}

void * game_of_life_thread (void *ptr) {
  thread_args *ta = (thread_args *) ptr;
  int start = ta->start;
  int end = ta->end;
  int ncols = ta->ncols;
  int nrows = ta->nrows;
  char * inboard = ta->inboard;
  char * outboard = ta->outboard;
  int i, j;

  for (j = 0; j < ncols; j++) {
    const int j_nrows = j * nrows;

    for (i = start; i < end; i++) {
      char cell = BOARD(inboard,i,j);
      // Check if the cell can come to life
      if (!IS_ALIVE(cell)) {
        if (cell == (char) 0x3) {
          SET_ALIVE(BOARD(outboard,i,j));

          const int inorth = i ? i - 1 : nrows - 1;
          const int isouth = (i != nrows - 1) ? i + 1 : 0;
          const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
          const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

          // // Notify neighbours of spawn
          MY_INCREMENT_NEIGHBOURS (outboard, inorth, jwest);
          MY_INCREMENT_NEIGHBOURS (outboard, inorth, j_nrows);
          MY_INCREMENT_NEIGHBOURS (outboard, inorth, jeast);
          MY_INCREMENT_NEIGHBOURS (outboard, i, jwest);
          MY_INCREMENT_NEIGHBOURS (outboard, i, jeast);
          MY_INCREMENT_NEIGHBOURS (outboard, isouth, jwest);
          MY_INCREMENT_NEIGHBOURS (outboard, isouth, j_nrows);
          MY_INCREMENT_NEIGHBOURS (outboard, isouth, jeast);
        }
      } else {
        // Check if the cell needs to die
        if (cell <= (char) 0x11 || cell >= (char) 0x14) {
          SET_DEAD(MY_BOARD(outboard,i,j_nrows));

          // Notify neighbours of death
          const int inorth = mod (i-1, nrows);
          const int isouth = mod (i+1, nrows);
          const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
          const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

          MY_DECREMENT_NEIGHBOURS (outboard, inorth, jwest);
          MY_DECREMENT_NEIGHBOURS (outboard, inorth, j_nrows);
          MY_DECREMENT_NEIGHBOURS (outboard, inorth, jeast);
          MY_DECREMENT_NEIGHBOURS (outboard, i, jwest);
          MY_DECREMENT_NEIGHBOURS (outboard, i, jeast);
          MY_DECREMENT_NEIGHBOURS (outboard, isouth, jwest);
          MY_DECREMENT_NEIGHBOURS (outboard, isouth, j_nrows);
          MY_DECREMENT_NEIGHBOURS (outboard, isouth, jeast);
        }
      }
    }
  }

  pthread_exit(NULL);
}


void game_of_life_single_row (int i, int ncols, int nrows, char * inboard, char * outboard) {
  int j;
  for (j = 0; j < ncols; j++) {
    const int j_nrows = j * nrows;
    char cell = BOARD(inboard,i,j);
    // Check if the cell can come to life
    if (!IS_ALIVE(cell)) {
      if (cell == (char) 0x3) {
        SET_ALIVE(BOARD(outboard,i,j));

        // Notify neighbours of spawn
        const int inorth = INORTH(i, nrows);
        const int isouth = ISOUTH(i, nrows);
        const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
        const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

        MY_INCREMENT_NEIGHBOURS (outboard, inorth, jwest);
        MY_INCREMENT_NEIGHBOURS (outboard, inorth, j_nrows);
        MY_INCREMENT_NEIGHBOURS (outboard, inorth, jeast);
        MY_INCREMENT_NEIGHBOURS (outboard, i, jwest);
        MY_INCREMENT_NEIGHBOURS (outboard, i, jeast);
        MY_INCREMENT_NEIGHBOURS (outboard, isouth, jwest);
        MY_INCREMENT_NEIGHBOURS (outboard, isouth, j_nrows);
        MY_INCREMENT_NEIGHBOURS (outboard, isouth, jeast);
      }
    } else {
      // Check if the cell needs to die
      if (cell <= (char) 0x11 || cell >= (char) 0x14) {
        SET_DEAD(BOARD(outboard,i,j));

        // Notify neighbours of death
        const int inorth = INORTH(i, nrows);
        const int isouth = ISOUTH(i, nrows);
        const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
        const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;

        MY_DECREMENT_NEIGHBOURS (outboard, inorth, jwest);
        MY_DECREMENT_NEIGHBOURS (outboard, inorth, j_nrows);
        MY_DECREMENT_NEIGHBOURS (outboard, inorth, jeast);
        MY_DECREMENT_NEIGHBOURS (outboard, i, jwest);
        MY_DECREMENT_NEIGHBOURS (outboard, i, jeast);
        MY_DECREMENT_NEIGHBOURS (outboard, isouth, jwest);
        MY_DECREMENT_NEIGHBOURS (outboard, isouth, j_nrows);
        MY_DECREMENT_NEIGHBOURS (outboard, isouth, jeast);
      }
    }
  }
}
