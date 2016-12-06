#ifndef _util_h
#define _util_h

#define INCREMENT_AT_COORD(_b, _i, _j) (_b[_i + _j]++)
#define DECREMENT_AT_COORD(_b, _i, _j) (_b[_i + _j]--)

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

static inline void
tell_neighbors_alive(board, i, j, inorth, isouth, jeast, jwest) {
	INCREMENET_AT_COORD(board, inorth, jwest);
	INCREMENET_AT_COORD(board, inorth, j);
	INCREMENET_AT_COORD(board, inorth, jeast);
	INCREMENET_AT_COORD(board, isouth, jwest);
	INCREMENET_AT_COORD(board, isouth, j);
	INCREMENET_AT_COORD(board, isouth, jeast);
	INCREMENET_AT_COORD(board, i, jwest);
	INCREMENET_AT_COORD(board, i, jeast);
}

static inline void
tell_neighbors_dead(board, i, j, inorth, isouth, jeast, jwest) {
	DECREMENT_AT_COORD(board, inorth, jwest);
	DECREMENT_AT_COORD(board, inorth, j);
	DECREMENT_AT_COORD(board, inorth, jeast);
	DECREMENT_AT_COORD(board, isouth, jwest);
	DECREMENT_AT_COORD(board, isouth, j);
	DECREMENT_AT_COORD(board, isouth, jeast);
	DECREMENT_AT_COORD(board, i, jwest);
	DECREMENT_AT_COORD(board, i, jeast);
}

#endif /* _util_h */
