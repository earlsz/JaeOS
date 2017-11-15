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

/* global variables from scheduler */
extern cpu_t TODStarted;

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

/******************** INTERRUPT HANDLER **************************/

void interruptHandler(){
  /* Acknowledge highest priority interrupt and then give control
      over to the scheduler */
  cpu_t startTime, endTime;
  int deviceNum;
  device_t *devReg;
  int index, status, tranStatus;
  int * semV;
  pcb_t * wait;
  state_PTR oldInt = (state_PTR) INT_OLDAREA;


  //STCK isn't in the uARM architecture so we use getTODLO instead
  //STCK(startTime); //save the time the interrupt handler started
  startTime = getTODLO(); //save the time the interrupt handler started

  int cause = oldInt->CP15_Control; /* which line caused interrupt? */
  /* single out possible interrupting lines */ 
  int linenum = 0;
  /* An interrupt line will always be on if in handler */

  /* get device number */
  int devicenum = getDeviceNumber(cause);
    switch(devicenum){

      case 2://interval timer
        //LDIT(INTERVALTIME);/* load 100 ms into interval timer*/
        // LDIT not defined in uARM architecture
        semV = (int*) &(semD[CLOCK_SEM]);
        while(headBlocked(semV) != NULL) {//unblock semds
			    wait = removeBlocked(semV);
			    //STCK(endTime);
          endTime = getTODLO();
			    if(wait != NULL){
				    insertProcQ(&readyQueue, wait);
				    // bill process for time in interrupt handler
				    (wait->cpu_time) = (wait->cpu_time) + (endTime - startTime);
				    --sftBlkCount;
			    }
          (*semV) = 0; //no one still blocked on the clock
          endProcForNow(startTime);
	}

      case 3://disk request
		    linenum = INT_DISK;
      case 4://tape interrupt
        linenum = INT_TAPE;
      case 5://network device
        linenum = INT_UNUSED;
      case 6://printer
        linenum = INT_PRINTER;
      case 7://terminal
        linenum = INT_TERMINAL;
      default:
        PANIC(); //just in case
      
    }

    // get actual register associated with that device
    devReg = (device_t *)(INTDEVREG + ((linenum-DEVWOSEM)
		  			* DEV_REG_SIZE * DEV_PER_INT) + (devicenum * DEV_REG_SIZE));
	
	  if(linenum != 7){ // not a terminal
		  //store device status
		  status = devReg -> d_status; 
      //ACK device
      devReg -> d_command = DEV_C_ACK;
		  //compute which semaphore to V
		  index = DEV_PER_INT * (linenum - DEVWOSEM) + devicenum;
		
	  } else { // terminal 
		  tranStatus = (devReg->t_transm_status);

		  if( tranStatus == 3 || tranStatus == 4 || tranStatus == 5 ) {
			  // write terminal 
			  index = (DEV_PER_INT * (linenum - DEVWOSEM)) + devicenum;
			  status = devReg->t_transm_status;
			  devReg->t_transm_command = DEV_C_ACK;
		  }
		  else {
			  // read terminal 
			  index = DEV_PER_INT * (linenum - DEVWOSEM + 1) + devicenum;
			  status = devReg->t_recv_status;
			  devReg->t_recv_command = DEV_C_ACK;
		  }
	  }

  /* V semaphore associated with that device */
  semV = &(semD[index]);
  ++(*semV);

  if((*semV) <= 0) {
    /* Wake up a process that was blocked */
    wait = removeBlocked(semV);
    if(wait != NULL){
      wait -> p_s.a2 = status;
      insertProcQ(&readyQueue, wait);
      --sftBlkCount;
    }
  } else {
    /* ERROR? */
  }
  /* exit interrupts */
  endProcForNow(startTime);
      
}

/********************* endProcForNow ****************************************/

void endProcForNow(cpu_t startTime){
  //blame it for the time it took up and move on
  cpu_t endTime;
  int linenum;
  state_t * oldInt = (state_t *)INT_OLDAREA;
  if(currProc != NULL){ /* was not in a wait state */
    /* take time used in interrupt handler and add it to TODstarted
     * so currProc is not billed for time
     */

    //STCK(endTime);
    endTime = getTODLO();
    TODStarted = TODStarted + (endTime-startTime);
    //return running process to ready queue
    copyState(oldInt, &(currProc->p_s));
    insertProcQ(&readyQueue, currProc);
  }
  scheduler();
}

/******************* GET DEVICE NUMBER FUNCTION ********************************/

int getDeviceNumber(unsigned int* bitMap) {
  /* Determines which device on an interrupt line is causing the interrupt
     and returns the position of the highest priority bit in that bitmap,
     starting with 0*/ 


     // WE DONT HAVE FIRSTDEVICE, SECONDDEVICE, etc. DEFINED ANYWHERE!!!
  unsigned int cause = *bitMap;
  if((cause & FIRSTDEVICE) != 0) {
    return 0;
  }

  else if((cause & SECONDDEVICE) != 0){
    return 1;
  }

  else if((cause & THIRDDEVICE) != 0){
    return 2;
  }

  else if((cause & FOURTHDEVICE) != 0){
    return 3;
  }

  else if((cause & FIFTHDEVICE) != 0){
    return 4;
  }

  else if((cause & SIXTHDEVICE) != 0){
    return 5;
  }

  else if((cause & SEVENTHDEVICE) != 0){
    return 6;
  }

  else if((cause & EIGHTHDEVICE) != 0){
    return 7;
  }

  return -1; //there is no bit that is on
}
