#ifndef __LIST_H__
#define __LIST_H__
#include <common.h>
#include <klib.h>

//#define CORRECTNESS_FIRST

#ifndef CORRECTNESS_FIRST
struct node {
  struct node *next, *pre;
  uintptr_t start, end;
};
struct node *head, *tail;
void list_init(uintptr_t pm_start, uintptr_t pm_end);
void* add_node(struct node* p, size_t size);
void delete_node(struct node *p);

#define BIAS sizeof(struct node)
#endif

void *balloc(int size);

#endif
