#include <list.h>

void list_init(uintptr_t pm_start, uintptr_t pm_end){
    head = (char *)pm_start;
    tail = (char *)(pm_end-BIAS);
    head->next = tail; head->pre = NULL;
    tail->next = NULL; tail->pre = head;
    head->start = pm_start; head->end = head->start+BIAS;
    tail->start = pm_end-BIAS; tail->end = tail->start+BIAS; 
}