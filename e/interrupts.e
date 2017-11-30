#ifndef INTERRUPTS
#define INTERRUPTS

/********************** INTERRUPTS.E *****************************/


#include "../h/types.h"

extern int waitFlag;
extern cpu_t startTOD;

extern void interruptHandler();
extern int getDeviceNumber(int lineNumber);
extern void handleTerminal(int devNumber);
extern void returnFromInterrupt();


/***************************************************************/

#endif
