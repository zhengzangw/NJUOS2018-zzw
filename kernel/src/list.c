#include <list.h>

void list_init(uintptr_t pm_start, uintptr_t pm_end) {
    head = (void *)pm_start;
    tail = (void *)(pm_end - BIAS);
    head->next = tail;
    head->pre = NULL;
    tail->next = NULL;
    tail->pre = head;
    head->start = pm_start;
    head->end = head->start + BIAS;
    tail->start = pm_end - BIAS;
    tail->end = tail->start + BIAS;
}

// Add a node after p and return content start ptr
void *add_node(struct node *p) {
    struct node *tmp;
    tmp = (void *)p->end;
    tmp->start = p->end;
    tmp->end = tmp->start + size + BIAS;
    tmp->pre = p;
    tmp->next = p->next;
    p->next->pre = tmp;
    p->next = tmp;
    return (void *)(tmp->start + BIAS);
}

void delete_node(struct node *p) {
    p->next->pre = p->pre;
    p->pre->next = p->next;
}