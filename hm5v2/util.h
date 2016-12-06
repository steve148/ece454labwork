#ifndef _util_h
#define _util_h

/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

//#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])
#define BOARD( __board, __i, __j )  (__board[(__i) + nrows*(__j)])
#define MY_BOARD( __board, __i, __j )  (__board[(__i) + (__j)])

// Check if a cell is alive or dead
#define IS_ALIVE(var) ((var) & (1<<(4)))

// Set cell states in the bit representation of cells
#define SET_ALIVE(var)  (var |=  (1 << (4)))
#define SET_DEAD(var)  (var &= ~(1 << (4)))

// Increase or decrease neighbour counts in the bit representation of cells
#define INCREMENT_NEIGHBOURS(__board, __i, __j )  (__board[(__i) + nrows*(__j)] ++ )
#define DECREMENT_NEIGHBOURS(__board, __i, __j )  (__board[(__i) + nrows*(__j)] -- )
#define MY_INCREMENT_NEIGHBOURS(__board, __i, __j )  (__board[(__i) + (__j)] ++ )
#define MY_DECREMENT_NEIGHBOURS(__board, __i, __j )  (__board[(__i) + (__j)] -- )

/**
 * C's mod ('%') operator is mathematically correct, but it may return
 * a negative remainder even when both inputs are nonnegative.  This
 * function always returns a nonnegative remainder (x mod m), as long
 * as x and m are both positive.  This is helpful for computing
 * toroidal boundary conditions.
 */
static inline int 
mod (int x, int m)
{
  return (x < 0) ? ((x % m) + m) : (x % m);
}

/**
 * Given neighbor count and current state, return zero if cell will be
 * dead, or nonzero if cell will be alive, in the next round.
 */
static inline char 
alivep (char count, char state)
{
  return (! state && (count == (char) 3)) ||
    (state && (count >= 2) && (count <= 3));
}

#endif /* _util_h */
