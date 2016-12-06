#ifndef _util_h
#define _util_h

// Check for if BOARD element is alive
#define IS_ALIVE(var)   ((var) & (1<<(4)))
#define SET_ALIVE(var)  (var |= (1 << (4)))
#define SET_DEAD(var)   (var &= ~(1 << (4)))

#define INCREMENT_AT_COORD(__b, __i, __j) (__b[(__i) + nrows*(__j)]++)
#define DECREMENT_AT_COORD(__b, __i, __j) (__b[(__i) + nrows*(__j)]--)

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
