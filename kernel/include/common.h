#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>
#include <debug.h>
#include <kmt.h>
#include <klib.h>

extern spinlock_t lock_print;
#define lprintf(format, ...) \
    kmt->spin_lock(&lock_print);\
    printf(format, __VA_ARGS__); \
    kmt->spin_unlock(&lock_print)
#define warning(format) \
    kmt->spin_lock(&lock_print);\
    printf(format); \
    kmt->spin_unlock(&lock_print)

#endif
