#ifndef pcb
#define pcb

/****************************************************************
 *                                                              *
 * This file will contain code for the allocation and           *
 * deallocation of ProcBlk's in a pcbFree list, code for        *
 * process queue maintenance, and code for process tree         *
 * maintenance.                                                 *
 *                                                              *
 ****************************************************************/

#include "/home/joseph/OpSys/JaeOs/h/types.h"
#include "/home/joseph/OpSys/JaeOs/h/const.h"
#include "/home/joseph/OpSys/JaeOs/e/pcb.e"


static pcb_t *pcbFree_h; //pointer to the head of the pcbFree list

/**************ALLOCATION AND DEALLOCATION OF PROCBLK'S************************************/

void freePcb(pcb_t *p){
  /*free up a pcb by putting it in the freelist.
    i.e. the pcb pointed to by p gets put on the pcbFree list*/
  insertProcQ(&pcbFree_h, p);
}

/******************************************************************************************/

pcb_t *allocPcb(){
  /* remove a pcb from the pcbFree list and initialize all of its fields
      and return a pointer to the removed pcb. 
     if the free list is empty, return NULL. */
  pcb_t *temp;
  temp = removeProcQ(&pcbFree_h);
  if(temp != NULL){
    temp->p_next = NULL;
    temp->p_previous = NULL;
    temp->p_prnt = NULL;
    temp->p_child = NULL;
    temp->p_next_sib = NULL;
    temp->p_previous_sib = NULL;
    temp->p_semAdd = NULL;
  }
  return temp;
}

/*******************PROCESS QUEUE MAINTENANCE**********************************************/

void initPcbs(){
  /* initialize the pcbFree list to contain all elements of the static
      array of MAXPROC ProcBlk's*/
  int i;
  static pcb_t procTable[MAXPROC];
  pcbFree_h = mkEmptyProcQ();
  for(i = 0; i<MAXPROC ; i++){
      freePcb(&procTable[i]);
    }
}

/*******************************************************************************************/

pcb_t *mkEmptyProcQ(){
  /*return pointer to the tail of an empty process queue i.e. NULL */
  return NULL;
}

/*******************************************************************************************/

int emptyProcQ(pcb_t *tp){
  /* given a tail pointer to our queue, return true if the queue is empty. Return false
    otherwise */
  return (tp == NULL);
}

/*******************************************************************************************/

void insertProcQ(pcb_t **tp, pcb_t *p){
  /*given a pointer to a pointer to the tail of a queue (tp), and a pointer to the new ProcBlk
  or node (p), insert the new node into the process queue, we have 3 cases: 0 nodes, 1 node, >1 node*/
  if(emptyProcQ(*tp)){
    //there are no nodes in the list, we make p the only one.
      (*tp) = p; 
      p->p_next = p;
      p->p_previous = p;
      return;
  }
  if((*tp)->p_next == (*tp)){
    /*there is one node in the list, we need to make p the tail, since
    //we insert at the tail, and set p->next and p->previous to be (*tp)
    //and vice versa*/
    p->p_next = (*tp);
    p->p_previous = (*tp);
    (*tp)->p_next = p;
    (*tp)->p_previous = p;
    (*tp) = p;//p is the new tail
    return;
  }
  //if we made it here then there is >1 node in the list
  p->p_next = (*tp)->p_next; //p will be the new tail, so its "p_next" will be the head
  p->p_next->p_previous = p;
  p->p_previous = (*tp);
  (*tp)->p_next = p;
  (*tp) = p;
}

/******************************************************************************************/

pcb_t *removeProcQ(pcb_t **tp){
  /* remove and return pointer to the first (i.e. head) node in the process queue.*/
  if(emptyProcQ(*tp)){
    //case1: queue is empty
    return NULL;
  }
  if((*tp)->p_next == (*tp)){
    //case2: one node in the list
    pcb_t *nodeToReturn = (*tp);
    (*tp) = mkEmptyProcQ();
    return nodeToReturn;
  }
  //case3: if we made it here, there is at least 2 nodes in the list
  pcb_t *head = (*tp)->p_next;
  (*tp)->p_next->p_next->p_previous = (*tp);//set newhead->prevous to tp
  (*tp)->p_next = (*tp)->p_next->p_next;
  return head;
  
}

/******************************************************************************************/

pcb_t *outProcQ(pcb_t **tp, pcb_t *p){
  /*remove the procblk pointed to by p from the process queue. if the queue is empty or p is not
   found, return NULL.*/
  //case1: empty queue
  if(emptyProcQ((*tp))){
    return NULL;
  }
  //case2: there is only one ProcBlk in the queue
  if((*tp)->p_next == (*tp)){
    if((*tp) == p){
      pcb_t *nodeToReturn = (*tp);
      *tp = mkEmptyProcQ();
      return nodeToReturn;
    }
    //tp is the only node and it is not p, so p is not in the queue
    return NULL;
  }
  //case3: there is more than one node in the list.
  //case3a: tp is p and there are other nodes in the list
  if((*tp) == p){
    pcb_t *procToKill = (*tp);
    (*tp)->p_previous->p_next = (*tp)->p_next;//set the new tail node->next to be the head
    (*tp)->p_next->p_previous = (*tp)->p_previous;//set head->previous to the new tail
    (*tp) = (*tp)->p_previous;//new tail pointer
    return procToKill;
  }
  //case3b: tp is not p and there are more than 1 node on the queue
  pcb_t *maybeP = (*tp)->p_next;//we don't need to check tp because of case 3a,
                                //so start with p->next
  while(maybeP != p){
    if(maybeP == (*tp)){
       return NULL;//we made it back to tp, therefore p is not in the list
    }
    maybeP = maybeP->p_next; 
  }
  //if we make it here, then maybeP is equal to p, we need to remove it
  pcb_t *nextNode = maybeP->p_next;
  pcb_t *previousNode = maybeP->p_previous;
  previousNode->p_next = nextNode;
  nextNode->p_previous = previousNode;
  return maybeP;
  
}

/****************************************************************************************/

pcb_t *headProcQ(pcb_t *tp){
  /* return a pointer to the head of the process queue (i.e the first ProcBlk), 
     if the process queue is empty, return NULL*/
  if(emptyProcQ(tp)){ 
    return NULL;
  }
  //if there is one node, then we just return p_next which is the same as tp, so
  //2 of the cases: 1 node and more than one node, are the same.
  return tp->p_next;
}




/**********************PROCESS TREE MAINTENANCE******************************************/

int emptyChild(pcb_t *p){
  /* return TRUE if ProcBlk pointed to by p has no children */
  return (p->p_child == NULL);
}

/****************************************************************************************/

void insertChild(pcb_t *prnt, pcb_t *p){
  /* make the ProcBlk pointed to by p a child of the ProcBlk pointed to by prnt*/
  //case1: prnt has no children yet
  if(emptyChild(prnt)){
    prnt->p_child = p;
    p->p_prnt = prnt;
    return;
  }
  //case2: there are other children of prnt
  prnt->p_child->p_previous_sib = p;
  p->p_previous_sib = NULL;
  p->p_next_sib = prnt->p_child;
  prnt->p_child = p;
  p->p_prnt = prnt;
  
}

/***************************************************************************************/

pcb_t *removeChild(pcb_t *p){
  /* Kill the first child of the parent ProcBlk p. If p has no children,
     return NULL*/
  //case1: p has no children
  if(emptyChild(p)){
    return NULL;
  }
  //case2: p does have at least 1 child, we need to check if there are more.
  pcb_t *childToKill = p->p_child;
  //case2a: if p has no other children
  if(p->p_child->p_next_sib == NULL){
    p->p_child = NULL;
    return childToKill;
  }
  //case2b: p has other children, we make the next child replace the child we are killing
  p->p_child->p_next_sib->p_previous_sib = NULL;
  p->p_child = p->p_child->p_next_sib;
  return childToKill;
}

/****************************************************************************************/

pcb_t *outChild(pcb_t *p){
  /* make the child (i.e. ProcBlk) pointed to by p no longer a child of its parent */
  //case1: p has no parent
  if(p->p_prnt == NULL){
    return NULL;
  }
  //case2: p has no siblings
  if((p->p_next_sib == NULL) && (p->p_previous_sib == NULL)){
    p->p_prnt->p_child = NULL; //let the parent know their child is dead
    p->p_prnt = NULL; //you can't have parents if you're dead. 
    return p;
  }
  //case3: p is the first child
  if((p->p_next_sib != NULL) && (p->p_previous_sib == NULL)){
    return removeChild(p->p_prnt);
    //removeChild works here because it removes the first child in all cases
  }
  //case4: p is the last child
  if((p->p_next_sib == NULL) && (p->p_previous_sib != NULL)){
    p->p_previous_sib->p_next_sib = NULL;
    p->p_prnt = NULL;
    p->p_previous_sib = NULL;
    return p;
  }
  //case5: p is not the first or last child
  //if we made it here, that must be the case so no if statement needed.
  pcb_t *previousSib = p->p_previous_sib;
  pcb_t *nextSib = p->p_next_sib;
  nextSib->p_previous_sib = previousSib;
  previousSib->p_next_sib = nextSib;
  p->p_prnt = NULL;
  p->p_previous_sib = NULL;
  p->p_next_sib = NULL;
  return p;
}

#endif
