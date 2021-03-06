/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
    "Team Awesome",              /* Team name */

    "Rahul Chandan",     /* First member full name */
    "rahul.chandan@mail.utoronto.ca",  /* First member email address */

    "Lennox Stevenson",                   /* Second member full name (leave blank if none) */
    "lennox.stevenson@mail.utoronto.ca"                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/*
 * ECE 454 Students: Write your rotate functions here:
 */

#define PIXEL_STEP_SIZE 32

void transpose(int dim, pixel *src, pixel *dst)
{
    int i, j, k, l;
    for (i=0; i < dim; i+=PIXEL_STEP_SIZE)
    {
	for (j=0; j < dim; j+=PIXEL_STEP_SIZE)
	{
	    for (k=0; k < PIXEL_STEP_SIZE; k++)
	    {
		int ik = i + k;
		for (l=0; l < PIXEL_STEP_SIZE; l+=4)
		{
		    dst[RIDX(ik, j+l, dim)] = src[RIDX(j+l, ik, dim)];
		    dst[RIDX(ik, j+l+1, dim)] = src[RIDX(j+l+1, ik, dim)];
		    dst[RIDX(ik, j+l+2, dim)] = src[RIDX(j+l+2, ik, dim)];
		    dst[RIDX(ik, j+l+3, dim)] = src[RIDX(j+l+3, ik, dim)];
		}
	    }
	}
    }
}
void shuffle(int dim, pixel *dst)
{
    pixel temp;
    int i, j;
    for (i=0; i < dim/2; i++)
    {
	int dimi = dim-1-i;
        for (j=0; j < dim; j++)
        {
            temp = dst[RIDX(i, j, dim)];
            dst[RIDX(i, j, dim)] = dst[RIDX(dimi, j, dim)];
            dst[RIDX(dimi, j, dim)] = temp;
        }
    }
}
 
char transpose_and_shuffle_descr[] = "transpose_and_shuffle: As described in the lab report";
void transpose_and_shuffle(int dim, pixel *src, pixel *dst)
{
    transpose(dim, src, dst);
    shuffle(dim, dst);
}


/* 
 * second attempt (commented out for now)
 */
char rotate_two_descr[] = "second attempt";
void attempt_two(int dim, pixel *src, pixel *dst) 
{
    int stride = 32;
    int count = dim >> 5;
    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {
            int a3 = stride;
            do {
                *dst++ = *src;
                src += dim;
            } while(--a3);
            src -= dim * stride + 1;
            dst += dim - stride;
        } while(--a2);
        src += dim * (stride + 1);
        dst -= dim * dim - stride;
    } while(--a1);
}

/* 
 * third attempt (commented out for now)
 */
char rotate_three_descr[] = "third attempt";
void attempt_three(int dim, pixel *src, pixel *dst) 
{
    int stride = 16;
    int count = dim >> 4;
    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {
            int a3 = stride;
            do {
                *dst++ = *src;
                src += dim;
            } while(--a3);
            src -= dim * stride + 1;
            dst += dim - stride;
        } while(--a2);
        src += dim * (stride + 1);
        dst -= dim * dim - stride;
    } while(--a1);
}

/* 
 * third attempt (commented out for now)
 */
char rotate_five_descr[] = "fifth attempt";
void attempt_five(int dim, pixel *src, pixel *dst) 
{
    int stride = 8;
    int count = dim >> 3;
    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {
            int a3 = stride;
            do {
                *dst++ = *src;
                src += dim;
            } while(--a3);
            src -= dim * stride + 1;
            dst += dim - stride;
        } while(--a2);
        src += dim * (stride + 1);
        dst -= dim * dim - stride;
    } while(--a1);
}

/* 
 * fourth attempt (commented out for now)
 */
char rotate_four_descr[] = "fourth attempt";
void attempt_four(int dim, pixel *src, pixel *dst) 
{
    int stride = 16;
    int count = dim >> 4;
    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {

            *dst++ = *src;
            src += dim;
            *dst++ = *src;
            src += dim;
        	*dst++ = *src;
        	src += dim;
        	*dst++ = *src;
        	src += dim;
            *dst++ = *src;
            src += dim;
            *dst++ = *src;
            src += dim;
        	*dst++ = *src;
        	src += dim;
        	*dst++ = *src;
        	src += dim;
            *dst++ = *src;
            src += dim;
            *dst++ = *src;
            src += dim;
        	*dst++ = *src;
        	src += dim;
        	*dst++ = *src;
        	src += dim;
            *dst++ = *src;
            src += dim;
            *dst++ = *src;
            src += dim;
        	*dst++ = *src;
        	src += dim;
        	*dst++ = *src;
        	src += dim;

            src -= dim * stride + 1;
            dst += dim - stride;
        } while(--a2);
        src += dim * (stride + 1);
        dst -= dim * dim - stride;
    } while(--a1);
}

/* 
 * sixth attempt (commented out for now)
 */
char rotate_six_descr[] = "sixth attempt";
void attempt_six(int dim, pixel *src, pixel *dst) 
{
	int stride, count;
	if (dim == 1024) {
		stride = 32;
		count = dim >> 5;
	}
	else {
		stride = 16;
		count = dim >> 4;
	}

    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {
            int a3 = stride;
            do {
                *dst++ = *src;
                src += dim;
            } while(--a3);
            src -= dim * stride + 1;
            dst += dim - stride;
        } while(--a2);
        src += dim * (stride + 1);
        dst -= dim * dim - stride;
    } while(--a1);
}

/* 
 * seventh attempt (commented out for now)
 */
char rotate_seven_descr[] = "seventh attempt";
void attempt_seven(int dim, pixel *src, pixel *dst) 
{
    int stride = 16;
    int count = dim >> 4;

	int const1 = dim * stride + 1;
	int const2 = dim - stride;
	int const3 = const1 + dim - 1;
	int const4 = dim * dim - stride;

    src += dim - 1; 
    int a1 = count;
    do {
        int a2 = dim;
        do {
            int a3 = stride;
            do {
                *dst++ = *src;
                src += dim;
            } while(--a3);
            src -= const1;
            dst += const2;
        } while(--a2);
        src += const3;
        dst -= const4;
    } while(--a1);
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    attempt_three(dim, src, dst);
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    //add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    //add_rotate_function(&attempt_two, rotate_two_descr);   
    //add_rotate_function(&attempt_three, rotate_three_descr);   
    //add_rotate_function(&attempt_four, rotate_four_descr);   
    //add_rotate_function(&attempt_five, rotate_five_descr);   
    //add_rotate_function(&attempt_six, rotate_six_descr);
    add_rotate_function(&attempt_seven, rotate_seven_descr);   
    //add_rotate_function(&attempt_eight, rotate_eight_descr);   
    //add_rotate_function(&attempt_nine, rotate_nine_descr);   
    //add_rotate_function(&attempt_ten, rotate_ten_descr);   
    //add_rotate_function(&attempt_eleven, rotate_eleven_descr);   

    /* ... Register additional rotate functions here */
}

