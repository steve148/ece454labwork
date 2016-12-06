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

