/************************************************************************
 *
 * This file will handle interrupts. This file processes all types of 
 * device interrupts, inculding Interval Timer interrupts, and converting
 * device interrupts into V  operations on the appropriate semaphores.
 *
 **************************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/interrupts.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"

void debugX(int a, int b, int c, int d){
	int i = 73;
	i++;
}


void interruptHandler(){
		
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
	if(currentProcess != NULL){
		cpu_t stopTOD;
		
		/*Store ending TOD*/
		SETTIME(stopTOD);
		
		/*Store elapsed time*/
		elapsedTime = stopTOD - startTOD;
		currentProcess->p_time = currentProcess->p_time + elapsedTime;
		timeLeft = timeLeft - elapsedTime;
		
		/*Move the old state into the current process*/
		copyState(oldInt, &(currentProcess->p_s));
	}
		
	/*If the interrupt was an interval timer interrupt...*/
	if((pendingDevice & LINETWO) == LINETWO){
				
		/*If it was the interval timer...*/ 
		if(intTimerFlag || (timeLeft < 0)){
			
			/*Unblock the first process*/
			process = removeBlocked(&(semaphoreArray[CLCKTIMER]));
			
			/*While there is still a process to remove...*/
			while(process != NULL){
				
				process->p_semAdd = NULL;
				softBlockCount--;
				
				/*Add it to the ready queue*/
				insertProcQ(&(readyQueue), process);
				
				/*Remove the next process*/
				process = removeBlocked(&(semaphoreArray[CLCKTIMER]));
			}
			
			/*Set the seamphore to zero*/
			semaphoreArray[CLCKTIMER] = 0;

			/*Reload the interval timer*/
			setTIMER(QUANTUM);
			timeLeft = INTERVALTIME;
			intTimerFlag = FALSE;
					
			/*Leave the interrupt*/
			backToWhatWeWereDoing();
			
		}
		/*It was a process's quantum ending*/
		else{
		
			/*If there was a process running...*/
			if(currentProcess != NULL){
			
				/*Add the process back to the ready queue*/
				insertProcQ(&(readyQueue), currentProcess);
				currentProcess = NULL;
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
		semaphoreArray[deviceIndex] = semaphoreArray[deviceIndex] + 1;
		debugX(deviceIndex, devReg->devregbase, 9999, 8888);
		
		if(semaphoreArray[deviceIndex] <= 0){
			
			/*Unblock the next process*/
			process = removeBlocked(&(semaphoreArray[deviceIndex]));
			if(process != NULL){
				process->p_semAdd = NULL;
				
				/*Set status of interrupt for the waiting process*/
				process->p_s.a1 = dev->d_status;		
				softBlockCount--;
				
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
		backToWhatWeWereDoing();
	}
	
}


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
		semAdd = semAdd + DEVPERINT;
		receive = FALSE;
	}
	
	/*Increment semaphore address*/
	semaphoreArray[semAdd] = semaphoreArray[semAdd] + 1;
        debugX(semAdd, 6, 6, 6);
	debugX(semAdd, dev->t_transm_status, dev->t_transm_command, semaphoreArray[semAdd]);
	
	if(semaphoreArray[semAdd] <= 0){	
		
		/*Unblock the process*/
		process = removeBlocked(&(semaphoreArray[semAdd]));
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
			
			softBlockCount--;
			
			/*Add it to the ready queue*/
			debugX(dev->t_transm_status, process->p_s.a1, 3, 3);
			insertProcQ(&(readyQueue), process);
		}
	}
	
	/*Leave the interrupt*/
	backToWhatWeWereDoing();
}


void backToWhatWeWereDoing(){
	
	/*If processor wasn't waiting...*/
	if(currentProcess != NULL){
		/*Store start TOD*/
		SETTIME(startTOD);
		
		/*Continue where it left off*/
		LDST(&(currentProcess->p_s));
	}
	
	/*Otherwise, get a new job*/
	scheduler();
}
