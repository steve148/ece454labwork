#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS         8	// # of threads to use in parallelization
#define LOG2_NUM_THREADS    3	// log2(NUM_THREADS), used to determine chunk_size

/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { char* temp = b1; b1 = b2; b2 = temp; } while(0)

/*
 * All the necessary information to be passed to the threads
 */
typedef struct {
  char * inboard;
  char * outboard;
  int nrows;
  int ncols;
  int start;
  int end;
} thread_args;

/*
 * init_bitmap
 * Used to modify the starting board before going through the game of life iterations.
 */
void
init_bitmap (char * board, const int nrows, const int ncols){
  int i,j;
  for (i = 0; i < nrows * ncols; i++) {
    if (board[i] == 0x01)		// If element on board is alive (1)
      board[i] = board[i] << 4;		// Shift bits 4 to the left
  }

  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) { 	// For all elements in board

      /* Optimization: save a few multiplication operations
	 by strength reduction.
      */
      const int j_nrows = j * nrows;

      // If the cell is alive, notify all the other neighbours
      if (IS_ALIVE(BOARD(board, i, j))) {
        const int inorth = INORTH(i, nrows);
        const int isouth = ISOUTH(i, nrows);
        const int jwest = j ? j_nrows - nrows : (ncols - 1) * nrows;
        const int jeast = (j != ncols - 1) ? j_nrows + nrows : 0;
        MY_INCREMENT_NEIGHBOURS (board, inorth, jwest);
        MY_INCREMENT_NEIGHBOURS (board, inorth, j_nrows);
        MY_INCREMENT_NEIGHBOURS (board, inorth, jeast);
        MY_INCREMENT_NEIGHBOURS (board, i, jwest);
        MY_INCREMENT_NEIGHBOURS (board, i, jeast);
        MY_INCREMENT_NEIGHBOURS (board, isouth, jwest);
        MY_INCREMENT_NEIGHBOURS (board, isouth, j_nrows);
        MY_INCREMENT_NEIGHBOURS (board, isouth, jeast);
      }
    }
  }
}

/*
 * game_of_life_thread
 * Function called by pthread_create.
 * Goes through chunk of rows of to update the state of the board.
 */
void * game_of_life_thread (void *ptr) {
  // Get args from thread_args
  thread_args *ta = (thread_args *) ptr;
  int start = ta->start;
  int end = ta->end;
  int ncols = ta->ncols;
  int nrows = ta->nrows;
  char * inboard = ta->inboard;
  char * outboard = ta->outboard;
  int i, j;

  /*
   Optimization: Traverse by row the column since i represents columns and
   j represents rows when used in the macro BOARD.
  */
  for (j = 0; j < ncols; j++) {
    const int j_nrows = j * nrows;

    for (i = start; i < end; i++) {
      char cell = BOARD(inboard,i,j);
      if (!IS_ALIVE(cell)) {		// If cell is dead
        if (cell == (char) 0x3) {	// If cell is about to become alive
          SET_ALIVE(BOARD(outboard,i,j));

          const int inorth = INORTH(i, nrows);
          const int isouth = ISOUTH(i, nrows);
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
      } else {							// If cell is alive
        if (cell <= (char) 0x11 || cell >= (char) 0x14) {	// Cell will die from over or under population
          SET_DEAD(MY_BOARD(outboard,i,j_nrows));

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
  int num_threads = NUM_THREADS;
  pthread_t tid[NUM_THREADS];
  thread_args *tinfo = malloc(NUM_THREADS * sizeof(thread_args));

  init_bitmap(inboard, nrows, ncols);

  // Set up thread args
  int start_row = 0;
  int chunk_size = nrows >> LOG2_NUM_THREADS;
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
