/************************************************************************
 *
 * This file will handle interrupts. This file processes all types of 
 * device interrupts, inculding Interval Timer interrupts, and converting
 * device interrupts into V  operations on the appropriate semaphores.
 *
 **************************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/pcb.e"
#include "../e/asl.e"


#include "/usr/include/uarm/libuarm.h"

extern int procCount;
extern int sftBlkCount;
extern pcb_PTR currProc;
extern pcb_PTR readyQueue;
extern int semD[MAX_DEVICES];
extern int devStatus[MAX_DEVICES];
extern int intTimerFlag;
extern cpu_t timeLeft;
extern void debug(int a, int b, int c, int d);

/* global variables from scheduler */
extern cpu_t startTOD;

/********************* COPY STATE FUNCTION **********************************/

void copyState(state_PTR src, state_PTR dest){
  /*we will need to copy the state of the processor to somewhere
   when an interrupt occurrs*/
  dest -> a1 = src -> a1;
  dest -> a2 = src -> a2;
  dest -> a3 = src -> a3;
  dest -> a4 = src -> a4;
  dest -> v1 = src -> v1;
  dest -> v2 = src -> v2;
  dest -> v3 = src -> v3;
  dest -> v4 = src -> v4;
  dest -> v5 = src -> v5;
  dest -> v6 = src -> v6;
  dest -> sp = src -> sp;
  dest -> lr = src -> lr;
  dest -> pc = src -> pc;
  dest -> cpsr = src -> cpsr;
  dest -> CP15_Control = src -> CP15_Control;

}


void interruptHandler(){
		
	/*Local Variable Declarations*/
	int devNum;
	int lineNum;
	int deviceIndex;
	cpu_t elapsedTime;
	device_t* dev;
	pcb_PTR process;
	unsigned int currentStatus;
	state_t* oldInt = (state_t *) INTERRUPTOLDADDR;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	int pendingDevice = oldInt->CP15_Cause >> 24;
	
	/*Decrement pc to the instruction that was executing*/
	oldInt->pc = oldInt->pc - 4;
	
	/*If there was a process running...*/
	if(currProc != NULL){
		cpu_t stopTOD;
		
		/*Store ending TOD*/
		STCK(stopTOD);
		
		/*Store elapsed time*/
		elapsedTime = stopTOD - startTOD;
		currProc->cpu_time = currProc->cpu_time + elapsedTime;
		timeLeft = timeLeft - elapsedTime;
		
		/*Move the old state into the current process*/
		copyState(oldInt, &(currProc->p_s));
	}
		
	/*If the interrupt was an interval timer interrupt...*/
	if((pendingDevice & LINETWO) == LINETWO){
				
		/*If it was the interval timer...*/ 
		if(intTimerFlag || (timeLeft < 0)){
			
			/*Unblock the first process*/
			process = removeBlocked(&(semD[CLCKTIMER]));
			
			/*While there is still a process to remove...*/
			while(process != NULL){
				
				process->p_semAdd = NULL;
				sftBlkCount--;
				
				/*Add it to the ready queue*/
				insertProcQ(&(readyQueue), process);
				
				/*Remove the next process*/
				process = removeBlocked(&(semD[CLCKTIMER]));
			}
			
			/*Set the seamphore to zero*/
			semD[CLCKTIMER] = 0;

			/*Reload the interval timer*/
			setTIMER(QUANTUM);
			timeLeft = INTERVALTIME;
			intTimerFlag = FALSE;
					
			/*Leave the interrupt*/
			returnFromInterrupt();
			
		}
		/*It was a process's quantum ending*/
		else{
		
			/*If there was a process running...*/
			if(currProc != NULL){
			
				/*Add the process back to the ready queue*/
				insertProcQ(&(readyQueue), currProc);
				currProc = NULL;
			}
			
			setTIMER(QUANTUM);
			
			/*Get a new job*/
			scheduler();
		}
	}
	else{
		/*If the interrupt was a disk device interrupt...*/
		if((pendingDevice & LINETHREE) == LINETHREE){
			
			/*Get the device number*/
			devNum = getDeviceNumber(DISKINT);
			lineNum = DISKINT;
		}
		
		/*If the interrupt was a tape device interrupt...*/
		else if((pendingDevice & LINEFOUR) == LINEFOUR){
			
			/*Get the device number*/
			devNum = getDeviceNumber(TAPEINT);
			lineNum = TAPEINT;
		}
		
		/*If the interrupt was a network device interrupt...*/
		else if((pendingDevice & LINEFIVE) == LINEFIVE){
			
			/*Get the device number*/
			devNum = getDeviceNumber(NETWINT);
			lineNum = NETWINT;
		}
		
		/*If the interrupt was a printer device interrupt...*/
		else if((pendingDevice & LINESIX) == LINESIX){
			
			/*Get the device number*/
			devNum = getDeviceNumber(PRNTINT);
			lineNum = PRNTINT;
		}
		
		/*If the interrupt was a terminal device interrupt...*/
		else if((pendingDevice & LINESEVEN) == LINESEVEN){
			
			/*Get the device number*/
			devNum = getDeviceNumber(TERMINT);
			lineNum = TERMINT;
		}
		
		/*If the interrupt was a terminal interrupt...*/
		if(lineNum == TERMINT){
					
			/*Pass it to a helper*/
			handleTerminal(devNum);
		}
		
		/*Get the modified line number*/
		lineNum = lineNum - DISKINT;
		
		/*Get the index of the device*/
		deviceIndex = (DEVPERINT * lineNum) + devNum;
		
		/*Get the device generating the interrupt*/
		dev = (device_t *) (devReg->devregbase + (deviceIndex * DEVREGSIZE));
		
		/*Increment semaphore address*/
		semD[deviceIndex] = semD[deviceIndex] + 1;
		debug(deviceIndex, devReg->devregbase, 9999, 8888);
		
		if(semD[deviceIndex] <= 0){
			
			/*Unblock the next process*/
			process = removeBlocked(&(semD[deviceIndex]));
			if(process != NULL){
				process->p_semAdd = NULL;
				
				/*Set status of interrupt for the waiting process*/
				process->p_s.a1 = dev->d_status;		
				sftBlkCount--;
				
				/*Add it to the ready queue*/
				insertProcQ(&(readyQueue), process);
			}
			else{
				/*Set status of interrupt for the current process*/
				devStatus[deviceIndex] = dev->d_status;
			}
		}
		
		/*Acknowledge the command*/
		dev->d_command = ACK;
		
		/*Leave the interrupt*/
		returnFromInterrupt();
	}
	
}

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that gets the device number of the device generating an
 *interrupt. It examines the bit map and determines which bit is on and
 *the device generating the interrupt.
 *RETURNS: The number of the device on the line generating the interrupt
 **********************************************************************/
int getDeviceNumber(int lineNumber){
	
	/*Local Variable Declarations*/
	unsigned int *devBitMap;
	unsigned int currentDevice = DEVICEONE;
	int devNum = 0;
	int found = FALSE;
	
	/*Get the modified line number*/
	lineNumber = lineNumber - 3;
	
	/*Examine the bit map*/
	devBitMap = (unsigned int *) (INTBITMAPADDR + (lineNumber * DEVREGLEN));
	
	/*While the interrupting device hasn't been found...*/
	while(!found){
		
		/*If the bit is on...*/
		if((currentDevice & *devBitMap) == currentDevice){
			found = TRUE;
		}
		
		/*Move to the next bit*/
		else{
			currentDevice = currentDevice << 1;
			devNum++;
		}
	}
	
	/*Return the number of the device on the line*/
	return devNum;
}

/***********************************************************************
 *Function that handles terminal interrupts. It checks first to see if
 *the terminal was transmitting. Then it performs a V operation on the
 *semaphore for that device and acknowledges the interrupt.
 *RETURNS: N/a
 **********************************************************************/
void handleTerminal(int devNumber){	
	
	/*Local Variable Declarations*/
	pcb_PTR process;
	int semAdd = (TERMINT-DISKINT)*DEVPERINT + devNumber;
	int receive = TRUE;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* dev = (device_t *) (devReg->devregbase + (semAdd * DEVREGSIZE));
	
	/*If a character was transmitted and is not ready...*/
	if((dev->t_transm_status & 0x0F) != READY){
		
		/*Increase the semaphore device number by 8*/
		receive = FALSE;
	}
	
	/*Increment semaphore address*/
	semD[semAdd] = semD[semAdd] + 1;
        debug(semAdd, 6, 6, 6);
	debug(semAdd, dev->t_transm_status, dev->t_transm_command, semD[semAdd]);
	
	if(semD[semAdd] <= 0){	
		
		/*Unblock the process*/
		process = removeBlocked(&(semD[semAdd]));
		if(process != NULL){
			process->p_semAdd = NULL;
			
			/*If the device was receiving...*/
			if(receive){

				/*Acknowledge the read*/
				process->p_s.a1 = dev->t_recv_status;
				dev->t_recv_command = ACK;
			}
			
			/*The device was transmitting*/
			else{

				/*Acknowledge the transmit*/
				process->p_s.a1 = dev->t_transm_status;
				dev->t_transm_command = ACK;
			}
			
			sftBlkCount--;
			
			/*Add it to the ready queue*/
			insertProcQ(&(readyQueue), process);
		}
	}
	
	/*Leave the interrupt*/
	returnFromInterrupt();
}

/***********************************************************************
 *Function that checks to see if the processor was in a waiting state
 *before the interrupt and either loads the current process or calls to
 *the scheduler.
 *RETURNS: N/a
 **********************************************************************/
void returnFromInterrupt(){
	
	/*If processor wasn't waiting...*/
	if(currProc != NULL){
		/*Store start TOD*/
		STCK(startTOD);
		
		/*Continue where it left off*/
		LDST(&(currProc->p_s));
	}
	
	/*Otherwise, get a new job*/
	scheduler();
}
