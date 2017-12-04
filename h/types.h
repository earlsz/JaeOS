#ifndef TYPES
#define TYPES

#include "../h/const.h"

typedef signed int cpu_t;

typedef unsigned int memaddr;

typedef struct {
	unsigned int d_status;
	unsigned int d_command;
	unsigned int d_data0;
	unsigned int d_data1;
} device_t;

#define t_recv_status		d_status
#define t_recv_command		d_command
#define t_transm_status		d_data0
#define t_transm_command	d_data1

typedef struct {
	unsigned int rambase;
	unsigned int ramtop;
	unsigned int devregbase;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
} devregarea_t;

#define STATEREGNUM	22
typedef struct state_t {
	int	 			s_reg[STATEREGNUM];
} state_t, *state_PTR;

typedef struct pcb_t {
	struct pcb_t *p_next;
	struct pcb_t *p_previous;
	struct pcb_t *p_prnt;
	struct pcb_t *p_child;
	struct pcb_t *p_next_sib;
	struct pcb_t *p_previous_sib;
	state_PTR oldSys;
	state_PTR newSys;
	state_PTR oldPrgm;
	state_PTR newPrgm;
	state_PTR oldTlb;
	state_PTR newTlb;
	state_t p_s;
	cpu_t p_time;
	int *p_semAdd;
} pcb_t, *pcb_PTR;

#define	a1			s_reg[0]
#define	a2			s_reg[1]
#define a3			s_reg[2]
#define a4			s_reg[3]
#define v1			s_reg[4]
#define v2			s_reg[5]
#define v3			s_reg[6]
#define v4			s_reg[7]
#define v5			s_reg[8]
#define v6			s_reg[9]
#define sl			s_reg[10]
#define fp			s_reg[11]
#define ip			s_reg[12]
#define sp			s_reg[13]
#define lr			s_reg[14]
#define pc			s_reg[15]
#define cpsr			s_reg[16]
#define CP15_Control	s_reg[17]
#define CP15_EntryHi	s_reg[18]
#define CP15_Cause		s_reg[19]
#define todHI			s_reg[20]
#define todLO			s_reg[21]




#endif
