
/*********************SCHEDULER.C********************************************/
/* This file maintains a round-robin process scheduler that determines
 * job on the ready queue that needs to be ran. The scheduler also  performs
 * deadlock detection. In essemce, If no job is on the ready queue, we 
 * check the process count and soft block count to determine what action 
 * should be taken next. Else, we run the next job.
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

void scheduler(){

	pcb_PTR newJob = NULL;

	//Get next job on ready queue
	newJob = removeProcQ(&(readyQueue));
	
	/*if no jobs on ready queue*/
	if(newJob == NULL)
	{

		/*we have no current process*/
		currentProcess = NULL; 
		
		/*if all jobs are finished, we HALT */
		if(processCount == 0)
		{
			HALT();
		}
		/*there are jobs left*/
		else if(processCount > 0)
		{

			/* if softBlockCount = 0 but there are still
			   jobs left, we have hit deadlock, so we PANIC */
			if(softBlockCount == 0){											
				PANIC();
			}

			/*If there are processes blocked by I/0...*/
			if(softBlockCount > 0)
			{
				/*set the timer*/
				setTIMER(timeLeft);

				intTimerFlag = TRUE;

				/*Enable interrupts in the processor*/
				setSTATUS(getSTATUS() & ALLINTENABLED);
				WAIT();
			}
		}
	}
	
	/*we have a new job to process*/
	else
	{
		/*Process job*/
		processJob(newJob);
	}
}

/*Load the new job or current process and do timer stuff*/
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

/*perform a deep copy between two state_t structures for all registers*/
void copyState(state_t* source, state_t* target){

	int i;

	/*Copy the 22 registers*/
	for(i = 0; i < STATEREGNUM; i++){
		target->s_reg[i] = source->s_reg[i];
	} 
}
