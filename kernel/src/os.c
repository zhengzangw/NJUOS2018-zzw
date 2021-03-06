#include <common.h>
#include <os.h>

#ifdef DEBUG_LOCK
spinlock_t lock_debug;
#endif
spinlock_t lock_print;
spinlock_t lock_os;

int initialized = 0;
static void os_init() {
    // Init spinlock
#ifdef DEBUG_LOCK
    kmt->spin_init(&lock_debug, "debug");
#endif
    kmt->spin_init(&lock_print, "print");
    kmt->spin_init(&lock_os, "os");
    // Init module
    pmm->init();
    kmt->init();
    dev->init();
    vfs->init();
    initialized = 1;
}

static void os_run() {
    _intr_write(1);
    _yield();
    while (1);
}

callback_t handlers[MAXCB];
int h_handlers;

static void os_on_irq(int seq, int event, handler_t handler) {
    kmt->spin_lock(&lock_os);
    handlers[h_handlers++] = (callback_t){handler, event, seq};
    int i = h_handlers - 1;
    // Insert to make seq increase
    while (i && handlers[i].seq <= handlers[i - 1].seq) {
        callback_t tmp = handlers[i - 1];
        handlers[i - 1] = handlers[i];
        handlers[i] = tmp;
    }
    kmt->spin_unlock(&lock_os);
}

static _Context *os_trap(_Event ev, _Context *context) {
    assertIF0();
    // Special Check
    switch (ev.event) {
        case _EVENT_ERROR:
            warning("%s\n", ev.msg);
#ifdef DEBUG
            _halt(1);
#else
            return context;
#endif
    }
    kmt->spin_lock(&lock_os);
    // Call all valid handler
    _Context *ret = NULL;
    for (int i = 0; i < h_handlers; ++i) {
        Assert(i == h_handlers - 1 || handlers[i].seq <= handlers[i + 1].seq,
               "seq not increase");
        if (handlers[i].event == _EVENT_NULL || handlers[i].event == ev.event) {
            _Context *next = handlers[i].handler(ev, context);
            if (next) ret = next;
        }
    }
    kmt->spin_unlock(&lock_os);
    return ret;
}

MODULE_DEF(os){
    .init = os_init,
    .run = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
