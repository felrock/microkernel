// main.c
#include "kernel_hwdep.h"
#include "linked-list.h"
#include <limits.h>
// Variabels
uint TICK;
bool KERNEL_MODE;
TCB *Running;
// Data structures
list *rList;    // ready list
list *wList;    // waiting list
list *tList;    // timer list
//extra functions
void idle();
void addMessage_nb(mailbox *mb, char *pData, exception status);
void addMessage_b(mailbox *mb, char *pData, exception status);
msg *extractMessage(msg *message);
//code --->
int init_kernel(void){
  exception task ;
  set_ticks(0);
  KERNEL_MODE = INIT;
  rList = create_list();
  if(rList == NULL){
    return FAIL;
  }
  wList = create_list();
  if(wList == NULL){
    free(rList->pHead);
    free(rList->pTail);
    free(rList);
    return FAIL;
  }
  tList = create_list();
  if(tList == NULL){
    free(rList->pHead);
    free(rList->pTail);
    free(rList);
    free(wList->pHead);
    free(wList->pTail);
    free(wList);
    return FAIL;
  }
  task = create_task(idle, UINT_MAX);
  if(task == FAIL){
    free(rList->pHead);
    free(rList->pTail);
    free(rList);
    free(wList->pHead);
    free(wList->pTail);
    free(wList);
    free(tList->pHead);
    free(tList->pTail);
    free(tList);
    return FAIL;
  }
  return OK;
}
exception	create_task( void (*body)(), uint d ){
  listobj *tObj ;
  volatile exception FIRST_EXEC = TRUE;
  TCB *cTask = (TCB *) calloc(1,sizeof(TCB));
  if(cTask == NULL){
    return FAIL;
  }
  cTask->DeadLine = d;
  cTask->PC = body;
  cTask->SP = &(cTask->StackSeg[STACK_SIZE-1]);
  tObj = create_listobj(NULL);
  if(tObj == NULL){
    free(cTask);
    return FAIL;
  }
  tObj->pTask = cTask;
  if(KERNEL_MODE == INIT){
    insert_DeadLine(rList,tObj);
    Running = rList->pHead->pNext->pTask;
    return OK;
  }else{
    //isr_off();
    SaveContext();
    if(FIRST_EXEC){
      FIRST_EXEC = FALSE;
      insert_DeadLine(rList,tObj);
      Running = rList->pHead->pNext->pTask;
      LoadContext();
    }
  }
  return OK;
}
void terminate( void ){
  listobj *tObj = extract(rList->pHead->pNext);
  free(tObj->pTask);
  free(tObj);
  Running = rList->pHead->pNext->pTask;
  LoadContext();
}
void run( void ){
  timer0_start();
  KERNEL_MODE = RUNNING;
  //isr_on();
  LoadContext();
}
mailbox* create_mailbox( uint nMessages, uint nDataSize ){
  mailbox *mBox = (mailbox *)calloc(1,sizeof(mailbox));
  if(mBox == NULL){
    return NULL;
  }
  mBox->pTail = (msg *) calloc(1,sizeof(msg));
  if(mBox->pTail == NULL){
    free(mBox);
    return NULL;
  }
  mBox->pHead = (msg *) calloc(1,sizeof(msg));
  if(mBox->pHead == NULL){
    free(mBox->pTail);
    free(mBox);
    return NULL;
  }
  // set datasize and max messages
  mBox->nDataSize = nDataSize;
  mBox->nMaxMessages = nMessages;
  //link
  mBox->pHead->pNext = mBox->pTail;
  mBox->pHead->pPrevious = mBox->pHead;
  mBox->pTail->pNext = mBox->pTail;
  mBox->pTail->pPrevious = mBox->pHead;
  return mBox;
}
exception remove_mailbox( mailbox* mBox ){
  if(no_messages(mBox) == 0){
    free(mBox->pHead);
    free(mBox->pTail);
    free(mBox);
    return OK;
  }else{
    return NOT_EMPTY;
  }
}
exception no_messages( mailbox* mBox ){
  return mBox->nMessages + mBox->nBlockedMsg;
}

exception send_wait( mailbox* mBox, void* pData ){
  volatile exception FIRST_EXEC = TRUE;
  msg *message;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
    FIRST_EXEC = FALSE;
    //check if list is empty && first item status
    if(mBox->pHead->pNext != mBox->pTail && mBox->pHead->pNext->Status == RECEIVER){
      memcpy(mBox->pHead->pNext->pData,pData,mBox->nDataSize);
      //remove
      message = extractMessage(mBox->pHead->pNext);
      //increment messages
      mBox->nBlockedMsg--;
      insert_DeadLine(rList,extract(message->pBlock));
      Running = rList->pHead->pNext->pTask;
      free(message);
    }else{
      addMessage_b(mBox, pData, SENDER);
    }
  LoadContext();
  }else {
    if(Running->DeadLine == ticks()){
      //isr_off();
      //remove and free memory
      message = extractMessage(rList->pHead->pNext->pMessage);
      mBox->nBlockedMsg--;
      free(message);
      //isr_on();
      return DEADLINE_REACHED;
    }else{
      return OK;
    }
  }
  return OK;
}
exception receive_wait( mailbox* mBox, void* pData ){
  volatile exception FIRST_EXEC = TRUE;
  msg* message;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
    FIRST_EXEC = FALSE;
    if(mBox->pHead->pNext != mBox->pTail && mBox->pHead->pNext->Status == SENDER){
      memcpy(pData,mBox->pHead->pNext->pData,mBox->nDataSize);
      message = extractMessage(mBox->pHead->pNext);
      // if SENDER is not of block-type
      if(message->pBlock != NULL){
        mBox->nBlockedMsg--;
        insert_DeadLine(rList,extract(message->pBlock));
        Running = rList->pHead->pNext->pTask;
        free(message);
      }else{
        mBox->nMessages--;
        //free allocated msg
        free(message->pData);
        free(message);
       }
     }else{
       addMessage_b(mBox, pData, RECEIVER);
      }
    LoadContext();
  }else{
    if(Running->DeadLine == ticks()){
      //isr_off();
      //remove
      message = extractMessage(rList->pHead->pNext->pMessage);
      mBox->nMessages--;
      //free memory
      free(message->pData);
      free(message);
      //isr_on();
      return DEADLINE_REACHED;
    }else{
      return OK;
    }
  }
  return OK;
}

exception send_no_wait( mailbox* mBox, void* pData ){
  volatile exception FIRST_EXEC = TRUE;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
    msg * message;
    FIRST_EXEC = FALSE;
    if(mBox->pHead->pNext != mBox->pTail && mBox->pHead->pNext->Status == RECEIVER){
      memcpy(mBox->pHead->pNext->pData,pData,mBox->nDataSize);
      //remove
      message= extractMessage( mBox->pHead->pNext );
      insert_DeadLine(rList,extract(message->pBlock));
      mBox->nBlockedMsg--;
      free(message);
      Running = rList->pHead->pNext->pTask;
      LoadContext();
    }else{
      void *pNewData;
      pNewData = (void *)calloc(1,sizeof(mBox->nDataSize));
      memcpy(pNewData,pData,mBox->nDataSize);
      if(mBox->nMessages == mBox->nMaxMessages){
        msg * temp;
        temp = extractMessage(mBox->pHead->pNext);
        mBox->nMessages--;
        free(temp->pData);
        free(temp);
      }
      addMessage_nb(mBox,pNewData,SENDER);
    }
  }
  return OK;
}
int receive_no_wait( mailbox* mBox, void* pData ){
  volatile exception FIRST_EXEC = TRUE;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
    msg *message;
    FIRST_EXEC = FALSE;
    if(mBox->pHead->pNext != mBox->pTail && mBox->pHead->pNext->Status == SENDER){
      //remove
      memcpy(pData,mBox->pHead->pNext->pData,mBox->nDataSize);
      message = extractMessage(mBox->pHead->pNext);
      if(message->pBlock != NULL){
        mBox->nBlockedMsg--;
        insert_DeadLine(rList,extract(message->pBlock));
        free(message);
      }else{
        mBox->nMessages--;
        free(message->pData);
        free(message);
      }
      Running = rList->pHead->pNext->pTask;
      LoadContext();
    }
  }
  return OK;
}

// Timing
exception wait( uint nTicks ){
  volatile exception FIRST_EXEC = TRUE;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
  listobj *rTask ;
    FIRST_EXEC = FALSE;
    rTask = pop(rList);
    Running = rList->pHead->pNext->pTask;
    insert_nTCnt(tList,rTask,ticks() + nTicks);
    LoadContext();
  }else{
    if(Running->DeadLine == ticks()){
      return DEADLINE_REACHED;
    }else{
      return OK;
    }
  }
  return OK;
}
void TimerInt(void){
  set_ticks(ticks()+1);
  while(tList->pHead->pNext != tList->pTail && tList->pHead->pNext->nTCnt <= ticks()){
      listobj *obj = extract(tList->pHead->pNext);
      obj->nTCnt = 0;
      insert_DeadLine(rList,obj);
      Running = rList->pHead->pNext->pTask;
  }
  while(tList->pHead->pNext != tList->pTail && tList->pHead->pNext->pTask->DeadLine <= ticks()){
      listobj *obj = extract(tList->pHead->pNext);
      obj->nTCnt = 0;
      insert_DeadLine(rList,obj);
      Running = rList->pHead->pNext->pTask;
  }
  while(wList->pHead->pNext != wList->pTail && wList->pHead->pNext->pTask->DeadLine <= ticks()){
      listobj *obj = extract(wList->pHead->pNext);
      insert_DeadLine(rList,obj);
      Running = rList->pHead->pNext->pTask;
  }
}
void set_ticks( uint no_of_ticks ){
  TICK = no_of_ticks;
}
uint ticks( void ){
  return TICK;
}
uint deadline( void ){
  return Running->DeadLine;
}
void set_deadline( uint nNew ){
  volatile exception FIRST_EXEC = TRUE;
  //isr_off();
  SaveContext();
  if(FIRST_EXEC){
    FIRST_EXEC = FALSE;
    Running->DeadLine = nNew;
    insert_DeadLine(rList,pop(rList));
    Running = rList->pHead->pNext->pTask;
    LoadContext();
  }
}
void addMessage_nb(mailbox *mb, char *pData, exception status){
  msg *pTemp;
  msg *message;
  //check if message is of blocked-type
  message=(msg *)calloc(1,sizeof(msg));
  //write the message
  message->pData = pData;
  message->Status = status;
  message->pBlock = NULL;
  //add to mailbox
  pTemp = mb->pTail;
  message->pPrevious = pTemp->pPrevious;
  message->pNext = pTemp;
  pTemp->pPrevious = message;
  message->pPrevious->pNext = message;
  //increment messages
  mb->nMessages++;
}
 void addMessage_b(mailbox *mb, char *pData, exception status){
  listobj *tObj;
  msg *message;
  msg *pTemp;
  tObj = extract(rList->pHead->pNext);
  message=(msg *)calloc(1,sizeof(msg));
  //write the message
  message->pData = pData;
  message->Status = status;
  message->pBlock = tObj;
  tObj->pMessage = message;
  //add at tail
  pTemp = mb->pTail;
  message->pPrevious = pTemp->pPrevious;
  message->pNext = pTemp;
  pTemp->pPrevious = message;
  message->pPrevious->pNext = message;
  //increment messages
  mb->nBlockedMsg++;
  //insert obj into waitinglist
  insert_DeadLine(wList,tObj);
  //change running pointer
  Running = rList->pHead->pNext->pTask;
}
msg *extractMessage(msg *message){
  if(message == NULL)return NULL;
  message->pPrevious->pNext = message->pNext;
  message->pNext->pPrevious = message->pPrevious;
  message->pNext = NULL;
  message->pPrevious = NULL;
  return message;
}
void idle(void){
  while(1){
//    SaveContext();
//    TimerInt();
//    LoadContext();
  }
}
