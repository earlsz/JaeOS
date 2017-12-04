
/************************EXCEPTIONS.C************************************/
/*
* This file will contain functions that handle program trap exceptions,
* TLB trap exceptions, and system calls.
*/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"

 /* Function that handles a Program trap exception. All TLB trap exceptions 
  * are handled as pass up or die as a Program trap */
void pgmTrapHandler(){
	
	state_t* oldState = (state_t*) PROGTRPOLDADDR;
		
	/*Call a Program Trap Pass Up or Die*/
	passUpOrDie(PROGTRAP);
	
}


 /* Function that handles a TLB trap exception. All TLB trap exceptions 
  * are handled as pass up or die as a TLB trap. */
void tlbHandler(){
	
	state_t* oldState = (state_t*) TLBOLDADDR;
		
	/*Call a TLB Trap Pass Up or Die*/
	passUpOrDie(TLBTRAP);
	
}


/****************************SYSCALLHANDLER*****************************/
/* Function that handles system calls. If a call 1-8 is made in user
 * mode, then it is handles it as a program trap exception. If the 
 * program is in system mode and makes a call 1 through 8, then the 
 * proper function is called based on the call number. All calls over
 * 8 in either mode is handled as a system call exception. 
 */
void sysCallHandler(){
	/*Local Variable Declarations*/
	pcb_PTR newPcb;
	pcb_PTR process = NULL;
	int *semAdd;
	int semDev;
	cpu_t elapsedTime;
	cpu_t stopTOD;
	state_t *sysCallOld;
	state_t *progTrpOld;
	int system = FALSE;
	
	
	state_t* oldState = (state_t*) SYSCALLOLDADDR;
	int sysCallNum = oldState->a1;
	
	/*Increment PC by 8*/
	oldState->pc = oldState->pc + 8;
	
	/*Move the old state into the current process*/
	copyState(oldState, &(currentProcess->p_s));
	
	/*If the state was in system mode...*/
	if((currentProcess->p_s.cpsr & SYSTEMMODE) == SYSTEMMODE){
		system = TRUE;
	}
	
	/*If process is in system mode...*/
	if(system){
		
		/*Based on the type of Syscall...*/
		switch(sysCallNum){

			//sys1
			case CREATEPROCESS:		
				sysOne(oldState);
				break;
				
			//sys2
			case TERMINATEPROCESS:
			
				sysTwo(currentProcess);
				break;
				
			//sys3				
			case VERHOGEN:
						
				sysThree(oldState, process);
				break;

			//sys4
			case PASSEREN:
							
				sysFour(currentProcess, oldState, elapsedTime, timeLeft, stopTOD);
				break;
			
			//sys5
			case SESV:
				
				sysFive(oldState->a2); 
				break;
			
			//sys6
			case GETCPUTIME:
				sysSix(stopTOD, elapsedTime, timeLeft, currentProcess);
				break;
				
			//sys7			
			case WAITFORCLOCK:
				
				semDev = CLCKTIMER;
				
				/*Decrement semaphore address*/
				semaphoreArray[semDev] = semaphoreArray[semDev] - 1;
				if(semaphoreArray[semDev] < 0){
										
					/*Store ending TOD*/
					SETTIME(stopTOD);
					
					/*Store elapsed time*/
					elapsedTime = stopTOD - startTOD;
					currentProcess->p_time = currentProcess->p_time + elapsedTime;
					timeLeft = timeLeft - elapsedTime;
					
					/*Block the process*/
					insertBlocked(&(semaphoreArray[semDev]), currentProcess);
					currentProcess = NULL;
					softBlockCount++;
					
					/*Get a new job*/
					scheduler();
				}
				
				/*ERROR*/				
				PANIC();
				break;
				
			//sys8
			case WAITFORIO:
				
				/*Get the proper semaphore device number*/
				semDev = DEVPERINT*(oldState->a2 - DISKINT) + oldState->a3;
							
				/*If the terminal is a write terminal...*/					
				if(!(oldState->a4) && (oldState->a2 == TERMINT)){
					semDev = semDev + DEVPERINT;
				}
				
				/*Decrement semaphore address*/
				semaphoreArray[semDev] = semaphoreArray[semDev] - 1;

				if(semaphoreArray[semDev] < 0){
										
					/*Store ending TOD*/
					SETTIME(stopTOD);
					
					elapsedTime = stopTOD - startTOD;
					currentProcess->p_time = currentProcess->p_time + elapsedTime;
					timeLeft = timeLeft - elapsedTime;
					
					/*Block the process*/
					insertBlocked(&(semaphoreArray[semDev]), currentProcess);
					currentProcess = NULL;
					softBlockCount++;	
					
					/*Get a new job*/	
					scheduler();
				}
				else {
					currentProcess->p_s.a1 = devStatus[semDev];
					LDST(&(currentProcess->p_s));
				}
				break;
				
			//any other syscalls come here to die...				
			default:
				
				/*Call a Syscall Pass Up or Die*/
				passUpOrDie(SYSTRAP);
				break;
				
		}
	}
	
	/*If the process is in user mode and it was a Syscall 1-8...*/
	if(sysCallNum >= CREATEPROCESS && sysCallNum <= WAITFORIO){
		
		/*Get the new areas in memory*/
		sysCallOld = (state_t *) SYSCALLOLDADDR;
		progTrpOld = (state_t *) PROGTRPOLDADDR;
		
		/*Move state from sysCallOld to progTrpOld*/
		copyState(sysCallOld, progTrpOld);
			
		/*Set cause register to priviledged instruction*/
		progTrpOld->CP15_Cause = RESERVED;

		/*Pass up to Program Trap Handler*/
		pgmTrapHandler();
	}
	
	/*Syscall greater than 8 in either mode*/
	/*Call a Syscall Pass Up or Die*/
	passUpOrDie(SYSTRAP);
	
	}

/************SYSCALL FUNCTIONS*******************************************/
/* This function creates a child process of the current process. 
 * Returns SUCCESS in a1 if the process could be created, otherwise
 * it returns FAILURE in a1.
 */
void sysOne(state_t *oldState){

	/*create a new process*/
	pcb_t *newPcb = allocPcb();
				
	/*If the free list was not empty... has to be the case
	 *in order to create a new process */
	if(newPcb != NULL){
					
		/*Copy SUCCESS code into return register*/
		oldState->a1 = SUCCESS;

		processCount++;
					
		/*Copy the state from a1 to the new pcb*/
		copyState((state_t *) oldState->a2, &(newPcb->p_s));
					
		/*Make process a child of current process and 
		 *add it to the ready queue */
		insertChild(currentProcess, newPcb);
		insertProcQ(&(readyQueue), newPcb);	
	}
				
	/*The free list was empty*/
	else{
					
		/*Copy FAILURE code into return register*/
		oldState->a1 = FAILURE;
	}
				
	/*Return to current process*/				
	LDST(&(currentProcess->p_s));
}

/* This function terminates the current process and 
 * all of its children processes. After killing all 
 * processes it calls the scheduler to get a new job.
 */
void sysTwo(pcb_t *currentProcess){
		/*Recursively kill process and children*/
		recursiveKill(currentProcess);
		currentProcess = NULL;
				
		/*Get a new job*/
		scheduler();
}

/* VERHOGEN. This performs a V operation (signal)
 * on a specified semaphore. After signaling a 
 * semaphore, it returns to the current process.
 * 
 * The address of the semaphore to V should 
 * already be stored in a2.
 */
void sysThree(state_t *oldState, pcb_t *process){
	int *semAddress = (int *) oldState->a2;
				
	/*Increment semaphore address*/
	*semAddress = *semAddress + 1;
				
	if(*semAddress <= 0){
					
		/*Unblock the next process*/
		process = removeBlocked(semAddress);
		process->p_semAdd = NULL;
					
		/*Add it to the ready queue*/
		insertProcQ(&(readyQueue), process);
	}

	/*Return to current process*/
	LDST(&(currentProcess->p_s));
}

/* PASSEREN. This performs a P operation (wait)
 * on a specified semaphore. If the current
 * process is blocked, we call the scheduler
 * to load the next job. Otherwise, we return
 * to the current process
 *
 * The address of the semaphore to P should 
 * already be stored in a2.
 */
void sysFour(pcb_t *currentProcess,state_t *oldState, cpu_t elapsedTime, cpu_t timeLeft, cpu_t stopTOD){
	int *semAdd = (int *) oldState->a2;
				
	/*Decrement semaphore address*/
	*semAdd = *semAdd - 1;
												
	if(*semAdd < 0){
										
		/*Store ending TOD*/
		SETTIME(stopTOD);
					
		/*Store elapsed time*/
		elapsedTime = stopTOD - startTOD;
		currentProcess->p_time = currentProcess->p_time + elapsedTime;
		timeLeft = timeLeft - elapsedTime;
					
		/*Block the currentProcess*/
		insertBlocked(semAdd, currentProcess);
		currentProcess = NULL;
										
		/*Get a new job*/
		scheduler();
	}
				
	/*Return to current process*/
	LDST(&(currentProcess->p_s));
}

/* This function sets the handlers for the current
 * process that are used in passUpOrDie. If the 
 * process already has a handler, the process and
 * all its children are killed. Otherwise, the
 * process is reloaded with the set handler in 
 * place.
 */
void sysFive(int type){
	/*Get the old status*/
	state_t* oldState = (state_t*) SYSCALLOLDADDR;
	
	/*Based on the type of trap...*/
	switch(type){
		
		case TLBTRAP: //TLBTRAP = 0
		
			/*If the area hasn't already been set with a handler..*/
			if(currentProcess->oldTlb == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldTlb = (state_t *)oldState->a3;
				currentProcess->newTlb = (state_t *)oldState->a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			
		case PROGTRAP: //PROGTAP = 1
			
			/*If the area hasn't already been set with a handler..*/
			if(currentProcess->oldPrgm == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldPrgm = (state_t *)oldState->a3;
				currentProcess->newPrgm = (state_t *)oldState->a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			
		case SYSTRAP: //SYSTRAP = 2
			
			/*If the area hasn't already been set with a handler..*/
			if(currentProcess->oldSys == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldSys = (state_t *)oldState->a3;
				currentProcess->newSys = (state_t *)oldState->a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
	}
	
	/*Kill the job and get a new one*/
	recursiveKill(currentProcess);
	scheduler();
}

/* This function returns the current process's
 * elapsed time on the cpu. The process's time
 * is stored in a1 and we return to our process.
 */
void sysSix(cpu_t stopTOD, cpu_t elapsedTime, cpu_t timeLeft, pcb_t *currentProcess){
	/*Store ending TOD*/
	SETTIME(stopTOD);
				
	/*Store elapsed time*/
	elapsedTime = stopTOD - startTOD;
	currentProcess->p_time = currentProcess->p_time + elapsedTime;
	timeLeft = timeLeft - elapsedTime;
				
	/*Copy current process time into return register*/		
	currentProcess->p_s.a1 = currentProcess->p_time;
				
	/*Store starting TOD*/
	SETTIME(startTOD);
				
	/*Return to current process*/
	LDST(&(currentProcess->p_s));
}

/* This function runs a program's exception handler if one has
 * been defined after a sys5 call was made. If no handler has
 * has been defined, then the process and all of its children
 * are killed and the scheduler is called.
 */
void passUpOrDie(int type){
	
	/*Based on the type of trap...*/
	switch(type){
		
		case PROGTRAP:
				
			/*If the handler has already been set up...*/
			if(currentProcess->oldPrgm != NULL){
				
				/*Move the areas around*/
				copyState((state_t *) PROGTRPOLDADDR, (currentProcess->oldPrgm));
				copyState(currentProcess->newPrgm, &(currentProcess->p_s));

				
				/*Return to the current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
		
		case TLBTRAP:
			
			/*If the handler has already been set up...*/
			if(currentProcess->oldTlb != NULL){
				
				/*Move the areas around*/
				copyState((state_t *) TLBOLDADDR, (currentProcess->oldTlb));
				copyState(currentProcess->newTlb, &(currentProcess->p_s));	
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
			
		case SYSTRAP:
		
			/*If the handler has already been set up...*/
			if(currentProcess->oldSys != NULL){
				
				/*Move the areas around*/
				copyState((state_t *) SYSCALLOLDADDR, (currentProcess->oldSys));
				copyState(currentProcess->newSys, &(currentProcess->p_s));
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
	
	}
	
	/*Kill the job and get a new one*/
	recursiveKill(currentProcess);
	scheduler();
}
	

/* This function kills a process and all of its children processes.
 * While doing this, softBlockCount, processCount, and semaphores
 * are updated accordingly.
 */
void recursiveKill(pcb_PTR parent){	
	
	/*While it has children...*/
	while(!emptyChild(parent)){
		/*Recursive death on child*/
		recursiveKill(removeChild(parent));
	}
	
	/*If the current process is the root...*/
	if (currentProcess == parent){
		outChild(parent);
	}
	
	/*If the process is on the ready queue...*/
	else if(parent->p_semAdd == NULL){
		outProcQ(&(readyQueue), parent);
	}
	
	else{
		/*Process is on the asl*/
		outBlocked(parent);
		
		/*If the pcb was on a device semaphore...*/
		if((parent->p_semAdd >= &(semaphoreArray[0])) && 
					(parent->p_semAdd <= &(semaphoreArray[CLCKTIMER]))){
						
			softBlockCount--;
		}
		/*The pcb is normally blocked*/
		else{
			
			/*Decrement semaphore address*/
			*(parent->p_semAdd) = *(parent->p_semAdd) + 1;
		}
	}
	
	/*Free the process block and decrement process count*/
	freePcb(parent);
	processCount--;
}
	
