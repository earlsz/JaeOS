#ifndef ASL
#define ASL

/************************** ASL.E *******************************/

#include "../h/types.h"

extern int insertBlocked (int *semAdd, pcb_PTR p);
extern pcb_PTR removeBlocked (int *semAdd);
extern pcb_PTR outBlocked (pcb_PTR p);
extern pcb_PTR headBlocked (int *semAdd);
extern void initASL ();

/***************************************************************/

#endif
