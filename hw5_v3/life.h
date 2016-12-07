#ifndef _life_h
#define _life_h

/**
 * Given the initial board state in inboard and the board dimensions
 * nrows by ncols, evolve the board state gens_max times by alternating
 * ("ping-ponging") between outboard and inboard.  Return a pointer to 
 * the final board; that pointer will be equal either to outboard or to
 * inboard (but you don't know which).
 */
char*
game_of_life (char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max);

/**
 * Same output as game_of_life() above, except this is not
 * parallelized.  Useful for checking output.
 */
char*
sequential_game_of_life (char* outboard, 
			 char* inboard,
			 const int nrows,
			 const int ncols,
			 const int gens_max);

// Function definitions for threaded applications. See life.c for full headers.
char * threaded_game_of_life(char* outboard, 
    char* inboard,
    const int nrows,
    const int ncols,
    const int gens_max);
void * game_of_life_thread (void *ptr);
void  game_of_life_single_row (int i, int ncols, int nrows, char * inboard, char * outboard);

void init_bitmap(char* board, const int nrows, const int ncols);
void terminate_bitmap(char* board, const int nrows, const int ncols);

#endif /* _life_h */
