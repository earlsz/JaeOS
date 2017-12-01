/* This file maintains a round-robin process scheduler and performs 
 * deadlock detection.
 */

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/interrupts.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"
extern void debugX(int a, int b, int c, int d);


void scheduler(){
	debugX(10, 11, 12, 13);
	pcb_PTR newJob = NULL;

	newJob = removeProcQ(&(readyQueue));
	
	if(newJob == NULL){
		
		currentProcess = NULL;
		
		if(processCount == 0){
			HALT();
		}	
		else if(processCount > 0){
			if(softBlockCount == 0){											
				PANIC();
			}

			/*If there are processes blocked by I/0...*/
			if(softBlockCount > 0){
				setTIMER(timeLeft);
				intTimerFlag = TRUE;
				/*Enable interrupts in the processor*/
				setSTATUS(getSTATUS() & ALLINTENABLED);
				WAIT();
			}
		}
	}
	
	else{
		/*Process job*/
		processJob(newJob);
	}
}

void processJob(pcb_PTR newJob){
		
	/*Set the current process to the new job*/
	currentProcess = newJob;
	
	/*Store starting TOD*/
	SETTIME(startTOD);
	
	if(timeLeft < 0) timeLeft = 0;
	
	/*If there is less than than one quantum left on the clock...*/
	if(timeLeft < QUANTUM){			
		
		/*Set the new job's timer to be the remaining interval time*/
		setTIMER(timeLeft);
		intTimerFlag = TRUE;
	}
	else{
		/*Set the new job's timer to be a full quantum*/
		setTIMER(QUANTUM);
	}
	
	/*Load the new job*/
	LDST(&(newJob->p_s));
	
}


void moveState(state_t* source, state_t* target){
	
	int i;
	
	/*Copy the 22 registers*/
	for(i = 0; i < STATEREGNUM; i++){
		target->s_reg[i] = source->s_reg[i];
	} 
}
