#ifndef SCHEDULER
#define SCHEDULER

/*********************** SCHEDULER.E ****************************/


#include "../h/types.h"


extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int waitFlag;
extern cpu_t startTOD;

extern void scheduler();
extern void processJob(pcb_PTR newJob);
extern void moveState(state_t *source, state_t *target);

/***************************************************************/

#endif
