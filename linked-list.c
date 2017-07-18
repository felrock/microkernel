//linked-list.c
#include "linked-list.h"
/*
    create list, allocate memory(list and dummy head/tail)
*/
list *create_list(){
  //allocate memory
  list *pList = (list *) calloc(1,sizeof(list));
  listobj *tListobj = (listobj *) calloc(1,sizeof(listobj));
  listobj *hListobj = (listobj *) calloc(1,sizeof(listobj));
  if(tListobj == NULL || pList == NULL || hListobj == NULL){
    free(tListobj);
    free(hListobj);
    free(pList);
    return NULL;
  }
  else{
	//add to head/tail list
	pList->pHead = hListobj;
	pList->pTail = tListobj;
    //connect head/tail
	pList->pHead->pNext = pList->pTail;
	pList->pHead->pPrevious = pList->pHead;
	pList->pTail->pNext = pList->pTail;
	pList->pTail->pPrevious = pList->pHead;
    return pList;
  }
}
/*
    create list, allocate memory(list and dummy head/tail)
*/
listobj *create_listobj(int num){
  listobj *prListobj = (listobj *) calloc(1,sizeof(listobj));
  if(prListobj == NULL){
    return NULL;
  }
  else{
    prListobj->nTCnt = num;
    return prListobj;
  }
}
/*
    Insert object into list, always first...
*/
void push(list *pList, listobj *pListobj){
  listobj *pTemp;
  pTemp = pList->pHead;

  pListobj->pNext = pTemp->pNext;
  pListobj->pPrevious = pTemp;
  pTemp->pNext = pListobj;
  pListobj->pNext->pPrevious = pListobj;
}
/*
    Insert object into list, lowest deadline first...
*/
void insert_DeadLine(list *pList, listobj *pListobj){
  listobj *pTemp;
  pTemp = pList->pHead;
  while(pTemp->pNext != pList->pTail){
    if(pTemp->pNext->pTask->DeadLine >= pListobj->pTask->DeadLine){
      break;
    }
    pTemp = pTemp->pNext;
  }
  pListobj->pNext = pTemp->pNext;
  pListobj->pPrevious = pTemp;
  pTemp->pNext = pListobj;
  pListobj->pNext->pPrevious = pListobj;
}
/*
    Insert object into list, lowest nTCnt first...
*/
void insert_nTCnt(list *pList, listobj *pListobj, uint ntcnt){
  listobj *pTemp;
  pTemp = pList->pHead;
  pListobj->nTCnt = ntcnt;
  while(pTemp->pNext != pList->pTail){
    if(pTemp->pNext->nTCnt >= pListobj->nTCnt){
      break;
    }
    pTemp = pTemp->pNext;
  }
  pListobj->pNext = pTemp->pNext;
  pListobj->pPrevious = pTemp;
  pTemp->pNext = pListobj;
  pListobj->pNext->pPrevious = pListobj;
}
/*
    un-connect object and return it..
*/
listobj *extract(listobj *pListobj){
	pListobj->pPrevious->pNext = pListobj->pNext;
	pListobj->pNext->pPrevious = pListobj->pPrevious;
	pListobj->pNext = NULL;
  pListobj->pPrevious = NULL;
	return pListobj;
}
/*
    un-connect object after head and return it..
*/
listobj *pop(list *pList){
  listobj *rNode ;
  if( pList->pHead->pNext == pList->pTail){return NULL;}
  rNode = pList->pHead->pNext;
	return extract(rNode);
}
