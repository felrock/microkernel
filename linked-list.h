#include "kernel.h"
#ifndef LIST_H
#define LIST_H
list *create_list();
listobj *create_listobj(int num);
void     insert(list *pList, listobj *pObj);
listobj *extract(listobj *pListobj);
listobj *extract_first(list *pList);
void     push(list *pList, listobj *pListobj);
listobj *pop(list *pList);
void     insert_DeadLine(list *pList, listobj *pListobj);
void     insert_nTCnt(list *pList, listobj *pListobj, uint ntcnt);
#endif
