#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/
#include "/usr/include/uarm/arch.h"
#include "/usr/include/uarm/uARMconst.h"


#define MAXPROC  		20
#define TRUE                    1
#define FALSE                   0
#define MAXINT                  0xFFFFFFFF
#define RAMSIZE                 0x000002D4
#define RAMBASE                 0x000002D0
#define ALLOFF                  0x00000000

/* everything below was added for phase2 */

/* system call codes */
#define	CREATETHREAD	        1	/* create thread */
#define	TERMINATETHREAD	        2	/* terminate thread */
#define	VERHOGEN		3	/* V a semaphore */
#define	PASSEREN		4	/* P a semaphore */
#define	SPECTRAPVEC		5	/* specify trap vectors for passing up */
#define	GETCPUTIME		6	/* get cpu time used to date */
#define	WAITCLOCK		7	/* delay on the clock semaphore */
#define	WAITIO			8	/* delay on a io semaphore */

#define CREATENOGOOD	-1

#define INTSDISABLED		0x000000C0

#define TLBTRAP			0
#define PROGTRAP		1
#define SYSTRAP	                2
#define DEVREGBASE              0x00000040

#define FIRSTDEVICE             0x01000000
#define SECONDDEVICE            0x02000000
#define THIRDDEVICE             0x04000000
#define FOURTHDEVICE            0X08000000
#define FIFTHDEVICE             0X10000000
#define SIXTHDEVICE             0X20000000
#define SEVENTHDEVICE           0X40000000
#define EIGHTHDEVICE            0X80000000

#define DEVWOSEM                2 //the first 2 devices are unused, we subtract this in certain places
#define INTDEVREG               0x00000040



#endif
