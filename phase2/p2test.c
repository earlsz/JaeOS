/* File: $Id: p2test.c,v 1.1 1998/01/20 09:28:08 morsiani Exp morsiani $ */ 

/*********************************P2TEST.C*******************************
 *
 *	Test program for the Kaya Kernel: phase 2.
 *
 *	Produces progress messages on Terminal0.
 *	
 *	This is pretty convoluted code, so good luck!
 *
 *		Aborts as soon as an error is detected.
 *
 *      Modified by Michael Goldweber on May 15, 2004
 *		Modified by Michael Goldweber: Fall 2017
 */

/* 
#include "../h/const.h"
#include "../h/types.h"
*/
#include "/usr/include/uarm/libuarm.h"
#include "/usr/include/uarm/uARMtypes.h"
#include "/usr/include/uarm/arch.h"
#include "/usr/include/uarm/uARMconst.h"

#define ALLOFF				0x00000000
#define INTSDISABLED		0x000000C0

#define TLBTRAP			0
#define PROGTRAP		1
#define SYSTRAP			2


typedef unsigned int devregtr;

#define cpu_t		int
#define EOS			'\0'
#define TIMESCALEADDR	BUS_REG_TIME_SCALE

/* hardware constants */
#define PRINTCHR	2
#define BYTELEN	8
#define RECVD	5

#define CLOCKINTERVAL	100000UL	/* interval to V clock semaphore */

#define TERMSTATMASK	0xFF
#define CAUSEMASK		0x0000001F
#define VMOFF 			0xF8FFFFFF

#define SYSCAUSE		(0x8 << 2)
#define RESVINSTR   	10
#define ADDRERROR		4

#define QPAGE			1024

#define IEPBITON		0x4
#define KUPBITON		0x8
#define KUPBITOFF		0xFFFFFFF7
#define TEBITON			0x08000000

#define CAUSEINTMASK	0xFD00
#define CAUSEINTOFFS	10

#define MINLOOPTIME		30000
#define LOOPNUM 		10000

#define CLOCKLOOP		10
#define MINCLOCKLOOP	3000	

#define BADADDR			0xFFFFFFFF
/* #define	TERM0ADDR		0x10000250 */
#define TERM0ADDR		DEV_REG_ADDR(7, 0)


/* Software and other constants */
#define PRGOLDVECT		4
#define TLBOLDVECT		2
#define SYSOLDVECT		6

/* system call codes */
#define	CREATETHREAD	1	/* create thread */
#define	TERMINATETHREAD	2	/* terminate thread */
#define	VERHOGEN		3	/* V a semaphore */
#define	PASSERN			4	/* P a semaphore */
#define	SPECTRAPVEC		5	/* specify trap vectors for passing up */
#define	GETCPUTIME		6	/* get cpu time used to date */
#define	WAITCLOCK		7	/* delay on the clock semaphore */
#define	WAITIO			8	/* delay on a io semaphore */

#define CREATENOGOOD	-1

/* just to be clear */
#define SEMAPHORE		int
#define NOLEAVES		4	/* number of leaves of p8 process tree */
#define MAXSEM			20



SEMAPHORE term_mut=1,	/* for mutual exclusion on terminal */
		s[MAXSEM+1],	/* semaphore array */
		testsem=0,		/* for a simple test */
		startp2=0,		/* used to start p2 */
		endp2=0,		/* used to signal p2's demise */
		endp3=0,		/* used to signal p3's demise */
		blkp4=1,		/* used to block second incaration of p4 */
		synp4=0,		/* used to allow p4 incarnations to synhronize */
		endp4=0,		/* to signal demise of p4 */
		endp5=0,		/* to signal demise of p5 */
		endp8=0,		/* to signal demise of p8 */
		endcreate=0,	/* for a p8 leaf to signal its creation */
		blkp8=0;		/* to block p8 */

state_t p2state, p3state, p4state, p5state,	p6state, p7state,p8rootstate, 
        child1state, child2state, gchild1state, gchild2state, gchild3state, gchild4state;

/* trap states for p5 */
state_t pstat_n, mstat_n, sstat_n, pstat_o,	mstat_o, sstat_o;

int		p1p2synch=0;	/* to check on p1/p2 synchronization */

int 	p8inc;			/* p8's incarnation number */ 
int		p4inc=1;		/* p4 incarnation number */

unsigned int p5Stack;	/* so we can allocate new stack for 2nd p5 */

int creation = 0; 				/* return code for SYSCALL invocation */
unsigned int *p5MemLocation = (unsigned int *)0x00000034;		/* To cause a p5 trap */

void	p2(),p3(),p4(),p5(),p5a(),p5b(),p6(),p7(),p7a(),p5prog(),p5mm();
void	p5sys(),p8root(),child1(),child2(),p8leaf();


/* a procedure to print on terminal 0 */
void print(char *msg) {

	char * s = msg;
	termreg_t * base = (termreg_t *) (TERM0ADDR);
	unsigned int status;
	
	SYSCALL(PASSERN, (int)&term_mut, 0, 0);				/* P(term_mut) */
	while (*s != EOS) {
		base->transm_command = PRINTCHR | (((unsigned int) *s) << BYTELEN);
		status = SYSCALL(WAITIO, IL_TERMINAL, 0, 0);	
		if ((status & TERMSTATMASK) != RECVD)
			PANIC();
		s++;	
	}
	SYSCALL(VERHOGEN, (int)&term_mut, 0, 0);				/* V(term_mut) */
}

/*                                                                   */
/*                 p1 -- the root process                            */
/*                                                                   */
void test() {	
	
	SYSCALL(VERHOGEN, (int)&testsem, 0, 0);					/* V(testsem)   */

	print("p1 v(testsem)\n");

	/* set up states of the other processes */

	/* set up p2's state */
	STST(&p2state);			/* create a state area             */	
	
	/* *** Should CP15_Control be explicitly set? *** */
	
	p2state.sp = p2state.sp - QPAGE;			/* stack of p2 should sit above    */
	p2state.pc = (unsigned int)p2;		/* p2 starts executing function p2 */
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
		

	STST(&p3state);

	p3state.sp = p2state.sp - QPAGE;
	p3state.pc = (unsigned int)p3;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	
	STST(&p4state);

	p4state.sp = p3state.sp - QPAGE;
	p4state.pc = (unsigned int)p4;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	
	STST(&p5state);
	
	p5Stack = p5state.sp = p4state.sp - (2 * QPAGE);	/* because there will 2 p4 running*/
	p5state.pc = (unsigned int)p5;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;

	STST(&p6state);
	
	p6state.sp = p5state.sp - (2 * QPAGE);
	p6state.pc = (unsigned int)p6;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	
	STST(&p7state);
	
	p7state.sp = p6state.sp - QPAGE;
	p7state.pc = (unsigned int)p7;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;

	STST(&p8rootstate);
	p8rootstate.sp = p7state.sp - QPAGE;
	p8rootstate.pc = (unsigned int)p8root;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
    
	STST(&child1state);
	child1state.sp = p8rootstate.sp - QPAGE;
	child1state.pc = (unsigned int)child1;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	STST(&child2state);
	child2state.sp = child1state.sp - QPAGE;
	child2state.pc = (unsigned int)child2;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	STST(&gchild1state);
	gchild1state.sp = child2state.sp - QPAGE;
	gchild1state.pc = (unsigned int)p8leaf;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;

	STST(&gchild2state);
	gchild2state.sp = gchild1state.sp - QPAGE;
	gchild2state.pc = (unsigned int)p8leaf;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	STST(&gchild3state);
	gchild3state.sp = gchild2state.sp - QPAGE;
	gchild3state.pc = (unsigned int)p8leaf;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	STST(&gchild4state);
	gchild4state.sp = gchild3state.sp - QPAGE;
	gchild4state.pc = (unsigned int)p8leaf;
	p2state.cpsr = ALLOFF | STATUS_SYS_MODE;
	
	
	/* create process p2 */
	SYSCALL(CREATETHREAD, (int)&p2state,0 , 0);				/* start p2     */

	print("p2 was started\n");

	SYSCALL(VERHOGEN, (int)&startp2, 0, 0);					/* V(startp2)   */

	SYSCALL(PASSERN, (int)&endp2, 0, 0);					/* P(endp2)     */

	/* make sure we really blocked */
	if (p1p2synch == 0)
		print("error: p1/p2 synchronization bad\n");

	SYSCALL(CREATETHREAD, (int)&p3state, 0, 0);				/* start p3     */

	print("p3 is started\n");

	SYSCALL(PASSERN, (int)&endp3, 0, 0);					/* P(endp3)     */

	SYSCALL(CREATETHREAD, (int)&p4state, 0, 0);				/* start p4     */

	SYSCALL(CREATETHREAD, (int)&p5state, 0, 0); 			/* start p5     */

	SYSCALL(CREATETHREAD, (int)&p6state, 0, 0);				/* start p6		*/

	SYSCALL(CREATETHREAD, (int)&p7state, 0, 0);				/* start p7		*/

	SYSCALL(PASSERN, (int)&endp5, 0, 0);					/* P(endp5)		*/ 

	print("p1 knows p5 ended\n");

	SYSCALL(PASSERN, (int)&blkp4, 0, 0);					/* P(blkp4)		*/

	/* now for a more rigorous check of process termination */
	for (p8inc=0; p8inc<4; p8inc++) {
		creation = SYSCALL(CREATETHREAD, (int)&p8rootstate, 0, 0);

		if (creation == CREATENOGOOD) {
			print("error in process termination\n");
			PANIC();
		}

		SYSCALL(PASSERN, (int)&endp8, 0, 0);
	}

	print("p1 finishes OK -- TTFN\n");
	* ((unsigned int *) BADADDR) = 0;				/* terminate p1 */

	/* should not reach this point, since p1 just got a program trap */
	print("error: p1 still alive after progtrap & no trap vector\n");
	PANIC();					/* PANIC !!!     */
}


/* p2 -- semaphore and cputime-SYS test process */
void p2() {
	int		i;				/* just to waste time  */
	cpu_t	now1,now2;		/* times of day        */
	cpu_t	cpu_t1,cpu_t2;	/* cpu time used       */
	cpu_t	targetTime;

	SYSCALL(PASSERN, (int)&startp2, 0, 0);				/* P(startp2)   */

	print("p2 starts\n");

	/* initialize all semaphores in the s[] array */
	for (i=0; i<= MAXSEM; i++)
		s[i] = 0;
	
	/* V, then P, all of the semaphores in the s[] array */
	for (i=0; i<= MAXSEM; i++)  {
		SYSCALL(VERHOGEN, (int)&s[i], 0, 0);			/* V(S[I]) */
		SYSCALL(PASSERN, (int)&s[i], 0, 0);			/* P(S[I]) */
		if (s[i] != 0)
			print("error: p2 bad v/p pairs\n");
	}


	print("p2 v's successfully\n");

	/* test of SYS6 */

	now1 = getTODLO();				/* time of day   */
	cpu_t1 = SYSCALL(GETCPUTIME, 0, 0, 0);			/* CPU time used */

	/* delay for several milliseconds */
	for (i=1; i < LOOPNUM; i++)
		;

	cpu_t2 = SYSCALL(GETCPUTIME, 0, 0, 0);			/* CPU time used */
	now2 = getTODLO();				/* time of day  */
	
	targetTime = (MINLOOPTIME / (*((unsigned int *)BUS_REG_TIME_SCALE))); 

	if (((now2 - now1) >= (cpu_t2 - cpu_t1)) && ((cpu_t2 - cpu_t1) >= targetTime))
		print("p2 is OK\n");
	else  {
		if ((now2 - now1) < (cpu_t2 - cpu_t1))
			print ("error: more cpu time than real time\n");
		if ((cpu_t2 - cpu_t1) < targetTime)
			print ("error: not enough cpu time went by\n");
		print("p2 blew it!\n");
	}

	p1p2synch = 1;				/* p1 will check this */

	SYSCALL(VERHOGEN, (int)&endp2, 0, 0);				/* V(endp2)     */

	SYSCALL(TERMINATETHREAD, 0, 0, 0);			/* terminate p2 */

	/* just did a SYS2, so should not get to this point */
	print("error: p2 didn't terminate\n");
	PANIC();					/* PANIC!           */
}


/* p3 -- clock semaphore test process */
void p3() {
	cpu_t	time1, time2;
	cpu_t	cpu_t1,cpu_t2;		/* cpu time used       */
	cpu_t	targetTime;
	int		i;

	time1 = 0;
	time2 = 0;

	/* loop until we are delayed at least half of clock V interval */
	while (time2-time1 < (CLOCKINTERVAL >> 1) )  { 
		time1 = getTODLO();			/* time of day     */
		print ("p3 - Timer Started\n");
		SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();			/* new time of day */
	}

	print("p3 - WAITCLOCK OK\n");

	/* now let's check to see if we're really charge for CPU
	   time correctly */
	cpu_t1 = SYSCALL(GETCPUTIME, 0, 0, 0);

	for (i=0; i<CLOCKLOOP; i++)
		SYSCALL(WAITCLOCK, 0, 0, 0); 
	
	cpu_t2 = SYSCALL(GETCPUTIME, 0, 0, 0);

	targetTime = (MINCLOCKLOOP / (*((unsigned int *)BUS_REG_TIME_SCALE))); 

	if ((cpu_t2 - cpu_t1) < targetTime)
		print("error: p3 - CPU time incorrectly maintained\n");
	else
		print("p3 - CPU time correctly maintained\n");


	SYSCALL(VERHOGEN, (int)&endp3, 0, 0);				/* V(endp3)        */

	SYSCALL(TERMINATETHREAD, 0, 0, 0);			/* terminate p3    */

	/* just did a SYS2, so should not get to this point */
	print("error: p3 didn't terminate\n");
	PANIC();					/* PANIC            */
}


/* p4 -- termination test process */
void p4() {
	switch (p4inc) {
		case 1:
			print("first incarnation of p4 starts\n");
			p4inc++;
			break;
		case 2:
			print("second incarnation of p4 starts\n");
			break;
	}

	SYSCALL(VERHOGEN, (int)&synp4, 0, 0);				/* V(synp4)     */

	SYSCALL(PASSERN, (int)&blkp4, 0, 0);				/* P(blkp4)     */

	SYSCALL(PASSERN, (int)&synp4, 0, 0);				/* P(synp4)     */

	/* start another incarnation of p4 running, and wait for  */
	/* a V(synp4). the new process will block at the P(blkp4),*/
	/* and eventually, the parent p4 will terminate, killing  */
	/* off both p4's.                                         */

	p4state.sp -= QPAGE;		/* give another page  */

	SYSCALL(CREATETHREAD, (int)&p4state, 0, 0);			/* start a new p4    */

	SYSCALL(PASSERN, (int)&synp4, 0, 0);				/* wait for it       */

	print("p4 is OK\n");

	SYSCALL(VERHOGEN, (int)&endp4, 0, 0);				/* V(endp4)          */

	SYSCALL(TERMINATETHREAD, 0, 0, 0);			/* terminate p4      */

	/* just did a SYS2, so should not get to this point */
	print("error: p4 didn't terminate\n");
	PANIC();					/* PANIC            */
}



/* p5's program trap handler */
void p5prog() {
	unsigned int exeCode = pstat_o.CP15_Cause & CAUSEMASK;
	switch (exeCode) {
	case BUSERROR:
		print("Access non-existent memory\n");
		pstat_o.pc = (unsigned int)p5a;   /* Continue with p5a() */
		break;
		
	case EXC_RESERVEDINSTR:
		print("privileged instruction\n");
		/* return in kernel mode */
		pstat_o.cpsr = ALLOFF | STATUS_SYS_MODE;
		pstat_o.pc = (unsigned int)p5b;
		break;
				
	default:
		print("other program trap\n");
	}
	
	LDST(&pstat_o);
}

/* p5's memory management trap handler */
void p5mm() {
	print("memory management trap\n");
	/* VM off, user mode on */
	mstat_o.cpsr = ALLOFF | STATUS_USER_MODE;  
	mstat_o.CP15_Control = ALLOFF;
	mstat_o.pc = (unsigned int)p5b;  /* return to p5b */
	mstat_o.sp = p5Stack-QPAGE;				/* Start with a fresh stack */
	LDST(&mstat_o);
}

/* p5's SYS trap handler */
void p5sys() {
	unsigned int p5status = sstat_o.cpsr & STATUS_SYS_MODE;
	if (p5status == STATUS_SYS_MODE) {
		print("High level SYS call from kernel mode process\n");
	}
	else {
		print("High level SYS call from user mode process\n");
	}
	LDST(&sstat_o);
}

/* p5 -- SYS5 test process */
void p5() {
	print("p5 starts\n");

	/* set up higher level TRAP handlers (new areas) */
	STST(&pstat_n);
	pstat_n.pc = (unsigned int)p5prog;
	
	STST(&mstat_n);
	mstat_n.pc = (unsigned int)p5mm;
	
	STST(&sstat_n);
	sstat_n.pc = (unsigned int)p5sys;

	/* trap handlers should operate in complete mutex: no interrupts on */
	/* this because they must restart using some BIOS area */
	/* thus, IEP bit is not set for them (see test() for an example of it) */

	/* specify trap vectors */
	SYSCALL(SPECTRAPVEC, PROGTRAP, (int)&pstat_o, (int)&pstat_n);

	SYSCALL(SPECTRAPVEC, TLBTRAP, (int)&mstat_o, (int)&mstat_n);

	SYSCALL(SPECTRAPVEC, SYSTRAP, (int)&sstat_o, (int)&sstat_n);
	
	/* to cause a pgm trap access some non-existent memory */	
	*p5MemLocation = *p5MemLocation + 1;		 /* Should cause a program trap */
}

void p5a() {
	/* generage a TLB exception by turning on VM without setting up the seg tables */
	setCONTROL(CP15_VM_ON);
}

/* second part of p5 - should be entered in user mode */
void p5b() {
	cpu_t		time1, time2;

	SYSCALL(9, 0, 0, 0);
	/* the first time through, we are in user mode */
	/* and the P should generate a program trap */
	SYSCALL(PASSERN, (int)&endp4, 0, 0);			/* P(endp4)*/

	/* do some delay to be reasonably sure p4 and its offspring are dead */
	time1 = 0;
	time2 = 0;
	while (time2 - time1 < (CLOCKINTERVAL >> 1))  {
		time1 = getTODLO();
		SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();
	} 

	/* if p4 and offspring are really dead, this will increment blkp4 */

	SYSCALL(VERHOGEN, (int)&blkp4, 0, 0);			/* V(blkp4) */

	SYSCALL(VERHOGEN, (int)&endp5, 0, 0);			/* V(endp5) */

	/* should cause a termination       */
	/* since this has already been      */
	/* done for PROGTRAPs               */
	SYSCALL(SPECTRAPVEC, PROGTRAP, (int)&pstat_o, (int)&pstat_n);

	/* should have terminated, so should not get to this point */
	print("error: p5 didn't terminate\n");
	PANIC();				/* PANIC            */
}


/*p6 -- high level syscall without initializing trap vector*/
void p6() {
	print("p6 starts\n");

	SYSCALL(9, 0, 0, 0);		/* should cause termination because p6 has no 
			  trap vector */

	print("error: p6 alive after SYS9() with no trap vector\n");

	PANIC();
}

/*p7 -- program trap without initializing passup vector*/
void p7() {
	print("p7 starts\n");

	* ((unsigned int *) BADADDR) = 0;
		
	print("error: p7 alive after program trap with no trap vector\n");
	PANIC();
}


/* p8root -- test of termination of subtree of processes              */
/* create a subtree of processes, wait for the leaves to block, signal*/
/* the root process, and then terminate                               */
void p8root() {
	int		grandchild;

	print("p8root starts\n");

	SYSCALL(CREATETHREAD, (int)&child1state, 0, 0);

	SYSCALL(CREATETHREAD, (int)&child2state, 0, 0);

	for (grandchild=0; grandchild < NOLEAVES; grandchild++) {
		SYSCALL(PASSERN, (int)&endcreate, 0, 0);
	}
	
	SYSCALL(VERHOGEN, (int)&endp8, 0, 0);

	SYSCALL(TERMINATETHREAD, 0, 0, 0);
}

/*child1 & child2 -- create two sub-processes each*/

void child1() {
	print("child1 starts\n");
	
	SYSCALL(CREATETHREAD, (int)&gchild1state, 0, 0);
	
	SYSCALL(CREATETHREAD, (int)&gchild2state, 0, 0);

	SYSCALL(PASSERN, (int)&blkp8, 0, 0);
}

void child2() {
	print("child2 starts\n");
	
	SYSCALL(CREATETHREAD, (int)&gchild3state, 0, 0);
	
	SYSCALL(CREATETHREAD, (int)&gchild4state, 0, 0);

	SYSCALL(PASSERN, (int)&blkp8, 0, 0);
}

/*p8leaf -- code for leaf processes*/

void p8leaf() {
	print("leaf process starts\n");
	
	SYSCALL(VERHOGEN, (int)&endcreate, 0, 0);

	SYSCALL(PASSERN, (int)&blkp8, 0, 0);
}


