/*
* This file will contain syscalls for handling a number
* of different exception scenarios.
*/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"


void pgmTrapHandler(){
	
	state_t* oldState = (state_t*) PROGTRPOLDADDR;
		
	/*Call a Program Trap Pass Up or Die*/
	passUpOrDie(PROGTRAP);
	
}

/***********************************************************************
 *Function that handles a TLB trap exception. All TLB trap exceptions 
 *are handled as pass up or die as a TLB trap.
 **********************************************************************/
void tlbHandler(){
	
	state_t* oldState = (state_t*) TLBOLDADDR;
		
	/*Call a TLB Trap Pass Up or Die*/
	passUpOrDie(TLBTRAP);
	
}

void debug(int a, int b, int c, int d){ int i = 5;}

/****************************SYSCALLHANDLER*****************************/
void sysCallHandler(){
 	debug(1,2,3,4);
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
	int sysCallNum = oldState->s_a1;
	
	/*Increment PC by 8*/
	oldState->s_pc = oldState->s_pc + 8;
	
	/*Move the old state into the current process*/
	moveState(oldState, &(currentProcess->p_s));
	
	/*If the state was in system mode...*/
	if((currentProcess->p_s.s_cpsr & SYSTEMMODE) == SYSTEMMODE){
		system = TRUE;
	}
	
	/*If in system mode...*/
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
				
				sysFive(oldState->s_a2); 
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
				semDev = DEVPERINT*(oldState->s_a2 - DISKINT) + oldState->s_a3;
							
				/*If the terminal is a write terminal...*/					
				if(!(oldState->s_a4) && (oldState->s_a2 == TERMINT)){
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
					currentProcess->p_s.s_a1 = devStatus[semDev];
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
	
	/*The process was not in system mode*/
	
	/*If it was a Syscall 1-8...*/
	if(sysCallNum >= CREATEPROCESS && sysCallNum <= WAITFORIO){
		
		/*Get the new areas in memory*/
		sysCallOld = (state_t *) SYSCALLOLDADDR;
		progTrpOld = (state_t *) PROGTRPOLDADDR;
		
		/*Move state from sysCallOld to progTrpOld*/
		moveState(sysCallOld, progTrpOld);
			
		/*Set cause register to priviledged instruction*/
		progTrpOld->s_CP15_Cause = RESERVED;

		/*Pass up to Program Trap Handler*/
		pgmTrapHandler();
	}
	
	/*Syscall 9-255*/
	
	/*Call a Syscall Pass Up or Die*/
	passUpOrDie(SYSTRAP);
	
	
}

void sysOne(state_t *oldState){
	pcb_t *newPcb = allocPcb();
				
	/*If the free list was not empty...*/
	if(newPcb != NULL){
					
		/*Copy SUCCESS code into return register*/
		oldState->s_a1 = SUCCESS;

		processCount++;
					
		/*Copy the state from a1 to the new pcb*/
		moveState((state_t *) oldState->s_a2, &(newPcb->p_s));
					
		/*Make it a child of current process and add it to
		*the ready queue
		*/
		insertChild(currentProcess, newPcb);
		insertProcQ(&(readyQueue), newPcb);	
	}
				
	/*The free list was empty*/
	else{
					
		/*Copy FAILURE code into return register*/
		oldState->s_a1 = FAILURE;
	}
				
	/*Return to current process*/				
	LDST(&(currentProcess->p_s));
}

void sysTwo(pcb_t *currentProcess){
		/*Recursively kill process and children*/
		recursiveKill(currentProcess);
		currentProcess = NULL;
				
		/*Get a new job*/
		scheduler();
}

void sysThree(state_t *oldState, pcb_t *process){
	int *semAddress = (int *) oldState->s_a2;
				
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

void sysFour(pcb_t *currentProcess,state_t *oldState, cpu_t elapsedTime, cpu_t timeLeft, cpu_t stopTOD){
	int *semAdd = (int *) oldState->s_a2;
				
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

void sysFive(int type){
	/*Get the old status*/
	state_t* oldState = (state_t*) SYSCALLOLDADDR;
	
	/*Based on the type of trap...*/
	switch(type){
		
		case TLBTRAP:
		
			/*If the area hasn't already been populated...*/
			if(currentProcess->oldTlb == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldTlb = (state_t *)oldState->s_a3;
				currentProcess->newTlb = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			
		case PROGTRAP:
			
			/*If the area hasn't been already populated...*/
			if(currentProcess->oldPrgm == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldPrgm = (state_t *)oldState->s_a3;
				currentProcess->newPrgm = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			
		case SYSTRAP:
			
			/*If the area hasn't been already populated...*/
			if(currentProcess->oldSys == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldSys = (state_t *)oldState->s_a3;
				currentProcess->newSys = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
	}
	
	/*Kill the job and get a new one*/
	recursiveKill(currentProcess);
	scheduler();
}

void sysSix(cpu_t stopTOD, cpu_t elapsedTime, cpu_t timeLeft, pcb_t *currentProcess){
	/*Store ending TOD*/
	SETTIME(stopTOD);
				
	/*Store elapsed time*/
	elapsedTime = stopTOD - startTOD;
	currentProcess->p_time = currentProcess->p_time + elapsedTime;
	timeLeft = timeLeft - elapsedTime;
				
	/*Copy current process time into return register*/		
	currentProcess->p_s.s_a1 = currentProcess->p_time;
				
	/*Store starting TOD*/
	SETTIME(startTOD);
				
	/*Return to previous process*/
	LDST(&(currentProcess->p_s));
}


void passUpOrDie(int type){
	
	/*Based on the type of trap...*/
	switch(type){
		
		case PROGTRAP:
				
			/*If the handler has already been set up...*/
			if(currentProcess->oldPrgm != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) PROGTRPOLDADDR, (currentProcess->oldPrgm));
				moveState(currentProcess->newPrgm, &(currentProcess->p_s));

				
				/*Return to the current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
		
		case TLBTRAP:
			
			/*If the handler has already been set up...*/
			if(currentProcess->oldTlb != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) TLBOLDADDR, (currentProcess->oldTlb));
				moveState(currentProcess->newTlb, &(currentProcess->p_s));	
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
			
		case SYSTRAP:
		
			/*If the handler has already been set up...*/
			if(currentProcess->oldSys != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) SYSCALLOLDADDR, (currentProcess->oldSys));
				moveState(currentProcess->newSys, &(currentProcess->p_s));
				
				/*Return to current process*/
				LDST(&(currentProcess->p_s));
			}
			break;
	
	}
	
	/*Kill the job and get a new one*/
	recursiveKill(currentProcess);
	scheduler();
}
	


//what became of the little ones, anakin?
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
	
