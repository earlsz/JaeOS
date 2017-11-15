#ifndef INTERRUPTS
#define INTERRUPTS

/************************** INTERRUPTS.E ******************************
*
*  The externals declaration file for the Interrupts Module
*
*/

#include "../h/types.h"

extern void interruptHandler();
extern int getDeviceNumber(unsigned int* bitMap);


/********************************************************************/

#endif
