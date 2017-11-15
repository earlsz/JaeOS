#ifndef EXCEPTIONS
#define EXCEPTIONS

/************************** EXCEPTIONS.E ******************************
*
*  The externals declaration file for the Exceptions Module
*
*/

#include "../h/types.h"

extern void copyState();
extern void tlbManager();
extern void pgmTrapHandler();
extern void syscallHandler();


extern void passUpOrDie(state_PTR caller, int reason);


/********************************************************************/

#endif