#ifndef __LIST_H__
#define __LIST_H__
#include <common.h>

struct node {
  struct node *next, *pre;
  uintptr_t start, end;
};
struct node *head, *tail;

#define BIAS sizeof(struct node)
#define Lognode(node) printf("Node: start=%p, end=%p\n", node->start, node->end)

#endif