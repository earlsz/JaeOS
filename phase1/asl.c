#ifndef asl
#define asl
/**********************************************************************
 *   This File contains code that initializes and maintains the       *
 *  active semaphore list, as well as a free list of blank semaphores *
 *  that will allow us to set a limit on the number of active         *
 *  semaphores possible. The free list of semaphores is a null        *
 *  terminated stack, with no initial nodes. The active semaphore     *
 *  list is also a queue, but it contains 2 nodes at initialization,  *
 *  1 represents the lowest value for a semadd, the other represents  *
 *  the highest value for a semadd, so we only ever insert or remove  *
 *  semaphores from the middle of the stack.                          *                                 
***********************************************************************/


#include "/home/joseph/OpSys/JaeOs/h/types.h"
#include "/home/joseph/OpSys/JaeOs/h/const.h"
#include "/home/joseph/OpSys/JaeOs/e/asl.e"
#include "/home/joseph/OpSys/JaeOs/e/pcb.e"

static semd_t *semd_h;
static semd_t *semdFree_h;

/*******************free list upkeep*************/

void insertFree(semd_t *semdesc){
  /*insert a semd onto the free list*/
  //if we make it here, then we simply add the new node at the top of the 
  //list even if the list is null, we can still just set the new node's 
  //s_next to be the current list, since the list is null terminated
  semdesc->s_next = semdFree_h;
  semdFree_h = semdesc;
}

semd_t  *removeFree(){
  /*remove the top node from the free list.*/
  semd_t *list = semdFree_h;
  //case1: the list is NULL, we need to return NULL
  if(list == NULL){
    return NULL;
  }
  //case 2: there is 1 or more nodes in the list. 
  semd_t *newTopNode = list->s_next;
  semd_t *nodeToReturn = list;
  semdFree_h = newTopNode;
  return nodeToReturn;
}

/*************active list upkeep***********************/

semd_t *findSem(semd_t **semlist, int *semadd){
  /*finds the semaphore that would be the parent of the semaphore with semadd, 
    regardless of wether a semaphore with that address exists or not*/
  semd_t *current = (*semlist);
  while(current->s_next->s_semAdd < semadd){
    current = current->s_next;
  }
  return current;
}

void insertSemd(semd_t **parent, semd_t *child){
  semd_t *mother = (*parent);
  child->s_next = mother->s_next;
  mother->s_next = child;
}

semd_t *removeSem(semd_t **parent){
  /*remove the semaphore with semadd from the active list and return it*/
  semd_t *current = (*parent);
  semd_t *semdesc = current->s_next;
  if(semdesc->s_semAdd == 0xFFFFFFFF){
    return NULL;
  }
  current->s_next = semdesc->s_next;
  return semdesc;
}

/*******************changing procQs for certain semd's**************/

int insertBlocked(int *semAdd, pcb_t *p){
  /*find the semaphore with semadd semadd, then add p to that semaphore's 
    process queue*/
  semd_t *parent = findSem(&semd_h, semAdd);
  semd_t *semdesc = parent->s_next;
  if(semdesc->s_semAdd != semAdd){
    //case1:there is no semaphore with semadd, we need to make a new one 
    //and then put p into its process queue
    semd_t *newSem = removeFree(semdFree_h);
    if(newSem == NULL){
      return TRUE;//inserting is blocked, so return true
    }
    p->p_semAdd = semAdd;
    newSem->s_semAdd = semAdd;
    newSem->s_procQ = mkEmptyProcQ();
    insertProcQ(&(newSem->s_procQ), p);
    insertSemd(&parent, newSem);
    return FALSE;
  }
  //case2: semdesc is not NULL, we found a semaphore with semadd
  //we simply insert p onto its proc queue
  p->p_semAdd = semAdd;
  insertProcQ(&(semdesc->s_procQ), p);
  return FALSE;//insert was not blocked, return false
}

pcb_t *removeBlocked(int *semAdd){
  /*search the asl for the semaphore with the semadd, remove the first 
    procblock on that semaphore's procq.*/
  semd_t *parent = findSem(&semd_h, semAdd);
  semd_t *semdesc = parent->s_next;
  if(semdesc->s_semAdd != semAdd){
    return NULL;
  }
  //if we made it here, semdesc exists. So we need to remove and return 
  //the first procBloc on semdesc's proc queue
  pcb_t *pcbToReturn =  removeProcQ(&(semdesc->s_procQ));
  if(emptyProcQ(semdesc->s_procQ)){
    //this means that the proc q of semdesc is empty, so we need to 
    //return semdesc to the free list.
    semd_t *removedSem = removeSem(&parent);
    removedSem->s_next = NULL;
    removedSem->s_semAdd = NULL;
    removedSem->s_procQ = NULL;
    insertFree(removedSem);
  }
  return pcbToReturn;
}

pcb_t *outBlocked(pcb_t *p){
  
  /* Finds a semaphore descriptor that has the process block p on it's 
     process queue. Removes P from that semaphore's process queue and
     returns it.*/
  
  semd_t *parent = findSem(&semd_h, p->p_semAdd);
  semd_t *semdesc = parent->s_next;
  pcb_t *pmaybe = outProcQ(&(semdesc->s_procQ), p);
  //its called pmaybe because it might be null
  //it doesn't matter if p is null for the purposes of this function, 
  //we return it anyway
  return pmaybe;
}

pcb_t *headBlocked(int *semAdd){
  /* find the first procblock in the procQ associated with semAdd 
     and return it, if the semadd or its procq are null, return NULL*/
  semd_t *parent = findSem(&semd_h, semAdd);
  semd_t *semdesc = parent->s_next;
  if(semdesc->s_procQ == NULL){
    return NULL;
  }
  return headProcQ(semdesc->s_procQ);
}

/**********************initialization********************************/

void initASL(){
  /*here we initialize the asl*/
  static semd_t dummyHead;//top of stack
  static semd_t dummyTail;//bottom of stack
  dummyHead.s_semAdd = 0;//minimum value
  dummyTail.s_semAdd = 0xFFFFFFFF;//maximum value
  dummyHead.s_next = &dummyTail;
  dummyTail.s_next = NULL;
  semd_h = &dummyHead;
  static semd_t semdTable[MAXPROC];
  semdFree_h = NULL;
  int i;
  //fill the free stack with blank semaphore descriptors.
  for(i = 0;i<MAXPROC;i++){
    insertFree(&semdTable[i]);
  }
}

#endif
