#ifndef TYPES
#define TYPES

/**************************************************************************** 
 *
 * This header file contains utility types definitions.
 * 
 ****************************************************************************/

#include "/usr/include/uarm/uARMtypes.h"

typedef signed int cpu_t;
typedef state_t  *state_PTR;

/* process table entry type */
typedef struct pcb_t {
	/* process queue fields */
  struct pcb_t   *p_next, /* ptr to next entry     */
                 *p_previous;

    /* process tree fields */
  struct pcb_t    *p_prnt, 		/* ptr to parent         */
		              *p_child,		/* ptr to 1st child      */
                  *p_next_sib, /* ptr to next sibling   */
                  *p_previous_sib;	        
	
    /* process status information */
  state_t	   p_s;			/* processor state       */
  int		     *p_semAdd;		/* ptr to semaphore on   */
					/* which proc is blocked */

  cpu_t cpu_time; /* time process spent on CPU in microsecs */
  state_PTR pgmTrpNew, pgmTrpOld, tlbNew, tlbOld, sysNew, sysOld;
  
}  pcb_t, *pcb_PTR;

typedef struct semd_t {
  struct semd_t *s_next; //next semaphore in the asl
  int *s_semAdd; //pointer to the address of the semaphore
  pcb_t *s_procQ; //a queue of procblocks
} semd_t;

typedef struct {
	unsigned int d_status;
	unsigned int d_command;
	unsigned int d_data0;
	unsigned int d_data1;
} device_t, *device_PTR;

typedef struct {
	unsigned int rambase;
	unsigned int ramtop;
	unsigned int devregbase;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
} devregarea_t;

#define t_recv_status		d_status
#define t_recv_command		d_command
#define t_transm_status		d_data0
#define t_transm_command	d_data1



#endif
