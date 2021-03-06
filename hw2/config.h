/*********************************************************
 * config.h - Configuration data for the driver.c program.
 *********************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 
 * CPEs for the baseline (naive) version of the rotate function that
 * was handed out to the students. Rd is the measured CPE for a dxd
 * image. Run the driver.c program on your system to get these
 * numbers.  
 */
#define R64    2.8 
#define R128   4.1 
#define R256   6.8
#define R512   10.9
#define R1024  13.5
#define R2048  29.4
#define R4096  36.9
#define R8192  44.7
#define R16384 53.6



/*
 * Baseline hitrates for 16k 2 way set assosiative cache 
 * NOT USED IN 2010
 */
#define C64   0.989       
#define C128  0.932       
#define C256  0.932       
#define C512  0.932       
#define C1024 0.932 
#define C2048  0.932
#define C4096  0.932
#define C8192  0.933
#endif /* _CONFIG_H_ */
