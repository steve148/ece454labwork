#include "load.h"
#include <assert.h>
#include <stdlib.h>
#include "util.h"
char*
make_board (const int nrows, const int ncols)
{
  char* board = NULL;
  int i;

  /* Allocate the board and fill in with 'Z' (instead of a number, so
     that it's easy to diagnose bugs */
  board = malloc (2 * nrows * ncols * sizeof (char));
  assert (board != NULL);
  for (i = 0; i < nrows * ncols; i++)
    board[i] = 'Z';

  return board;
}

static void
load_dimensions (FILE* input, int* nrows, int* ncols)
{
  int ngotten = 0;
  ngotten = fscanf (input, "P1\n%d %d\n", nrows, ncols);
  if (ngotten < 1)
    {
      fprintf (stderr, "*** Failed to read 'P1' and board dimensions ***\n");
      fclose (input);
      exit (EXIT_FAILURE);
    }
  if (*nrows < 1)
    {
      fprintf (stderr, "*** Number of rows %d must be positive! ***\n", *nrows);
      fclose (input);
      exit (EXIT_FAILURE);
    }
  if (*ncols < 1)
    {
      fprintf (stderr, "*** Number of cols %d must be positive! ***\n", *ncols);
      fclose (input);
      exit (EXIT_FAILURE);
    }
  if(*ncols * *nrows > 100000000) {
      fprintf (stderr, "*** World too big! ***\n");
      fclose (input);
      exit (EXIT_FAILURE);
  }
}

static char*
load_board_values (FILE* input, const int nrows, const int ncols)
{
  char* board = NULL;
  int ngotten = 0;
  int i = 0;

  /* Make a new board */
  board = make_board (nrows, ncols);

  /* Fill in the board with values from the input file */
  for (i = 0; i < nrows * ncols; i++)
  {
    ngotten = fscanf (input, "%c\n", &board[i]);
    if (ngotten < 1)
	  {
	    fprintf (stderr, "*** Ran out of input at item %d ***\n", i);
	    fclose (input);
	    exit (EXIT_FAILURE);
	  }
    else {
	    /* ASCII '0' is not zero; do the conversion */
	    board[i] = board[i] - '0';
      if (board[i] == 0x01)
      {
        board[i] = board[i] << 4;
      }
    }
  }

  init_neighbour_cnts(board, nrows, ncols);
  return board;
}

char*
load_board (FILE* input, int* nrows, int* ncols)
{
  load_dimensions (input, nrows, ncols);
  return load_board_values (input, *nrows, *ncols);
}

/*****************************************************************************
 * init_neighbour_cnts
 * Helper function to notify all neighbours about a cell that's alive
 ****************************************************************************/
void
init_neighbour_cnts (char * board, const int nrows, const int ncols){
  int i,j;
  for (i = 0; i < nrows; i++) {
    for (j = 0; j< ncols; j++) {
      // If the cell is alive, notify all the other neighbours
      if (IS_ALIVE(BOARD(board, i, j))) {
        const int inorth = mod (i-1, nrows);
        const int isouth = mod (i+1, nrows);
        const int jwest = mod (j-1, ncols);
        const int jeast = mod (j+1, ncols);
        INCREMENT_NEIGHBOURS (board, inorth, jwest);
        INCREMENT_NEIGHBOURS (board, inorth, j);
        INCREMENT_NEIGHBOURS (board, inorth, jeast);
        INCREMENT_NEIGHBOURS (board, i, jwest);
        INCREMENT_NEIGHBOURS (board, i, jeast);
        INCREMENT_NEIGHBOURS (board, isouth, jwest);
        INCREMENT_NEIGHBOURS (board, isouth, j);
        INCREMENT_NEIGHBOURS (board, isouth, jeast);
      }
    }
  }
}

