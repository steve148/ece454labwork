/*****************************************************************************
 * life.c
 * Freddy Chen 997363124
 * Anthony Alayo 997487401
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
  return threaded_gol (outboard, inboard, nrows, ncols, gens_max);
}

/*****************************************************************************
 * Package that we use to pass parameters to gol worker threads
 ****************************************************************************/
typedef struct Package {
  char * inboard;
  char * outboard;
  int nrows;
  int ncols;
  int start_row;
  int end_row;
} Package;

/*****************************************************************************
 * threaded_gol
 * Threaded version of the game of life
 ****************************************************************************/
  char*
threaded_gol (char* outboard, 
    char* inboard,
    const int nrows,
    const int ncols,
    const int gens_max)
{
  int curgen, i, j;

  // Set up threading variables
  int num_threads = 8;
  pthread_t tid[num_threads];
  Package *packages = malloc(num_threads*sizeof(Package));

  // Set up package for threads
  int start_row = 0;
  int chunk_size = nrows/num_threads;
  int end_row = chunk_size;
  for (i=0; i<num_threads; i++){
    packages[i].start_row = start_row+1;
    packages[i].end_row = end_row-1;

    // Update next start row and end row
    start_row = end_row;
    end_row = end_row + chunk_size;

    // Always need these
    packages[i].nrows = nrows;
    packages[i].ncols = ncols;
  }

  for (curgen = 0; curgen < gens_max; curgen++) {
    // Make sure the outboard and inboard is the same
    memmove (outboard, inboard, nrows * ncols * sizeof (char));

    // Do the first and last lines of each chunk so that threads don't
    // overlap writing to the outboard
    for (i = 0; i < num_threads; i++) {
      gol_worker_for_row( packages[i].start_row-1, ncols, nrows, inboard, outboard);
      gol_worker_for_row( packages[i].end_row, ncols, nrows, inboard, outboard);
    }

    // Do the rest of the chunk now
    for (i = 0; i < num_threads; i++) {
      packages[i].inboard = inboard;
      packages[i].outboard = outboard;
      pthread_create(&tid[i], NULL, gol_worker, (void*) &packages[i]);
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

  free(packages);
  return inboard;
}

/*****************************************************************************
 * gol_worker
 * Function for threads to run and update the inboard chunk size's states.
 * The cell's state is updated according to how many living neighbours it has.
 *
 * If a cell state changes, its neighbouring cells will be notified of an
 * updated count.
 ****************************************************************************/
void * gol_worker (void *ptr) {
  Package *p = (Package *) ptr;
  int start_row = p->start_row;
  int end_row = p->end_row;
  int ncols = p->ncols;
  int nrows = p->nrows;
  char * inboard = p-> inboard;
  char * outboard = p-> outboard;
  int i, j;

  for (j = 0; j < ncols; j++) {
    const int j_nrows = j * nrows;

    for (i = start_row; i < end_row; i++) {
      char cell = MY_BOARD(inboard,i,j_nrows);
      // Check if the cell can come to life
      if (!IS_ALIVE(cell)) {
        if (cell == (char) 0x3) {
          SET_ALIVE(MY_BOARD(outboard,i,j_nrows));

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


/*****************************************************************************
 * gol_worker_for_row
 * Special case of the above function in case we need to process a row by itself
 ****************************************************************************/
void gol_worker_for_row (int i, int ncols, int nrows, char * inboard, char * outboard) {
  int j;
  for (j = 0; j < ncols; j++) {
    const int j_nrows = j * nrows;
    char cell = BOARD(inboard,i,j);
    // Check if the cell can come to life
    if (!IS_ALIVE(cell)) {
      if (cell == (char) 0x3) {
        SET_ALIVE(BOARD(outboard,i,j));

        // Notify neighbours of spawn
        const int inorth = mod (i-1, nrows);
        const int isouth = mod (i+1, nrows);
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
