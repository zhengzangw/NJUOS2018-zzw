#include <common.h>
#include <klib.h>

typedef struct callback {
    handler_t handler;
    int event;
    int seq;
} callback_t;
#define MAXCB 256
