#ifndef INITIAL
#define INITIAL

/********************** INITIAL.E *******************************/

#include "../h/types.h"
#include "../h/const.h"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int waitFlag;
extern cpu_t startTOD;
extern int semaphoreArray[MAXSEMA];
extern int devStatus[MAXSEMA];
extern cpu_t timeLeft; 
extern int intTimerFlag;

extern void test();

extern int main();

/***************************************************************/

#endif
