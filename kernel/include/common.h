#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <vfs.h>
#include <nanos.h>
#include <debug.h>
#include <kmt.h>
#include <klib.h>
#include <common.h>
#include <pmm.h>
#include <util.h>

extern spinlock_t lock_print;
#define lprintf(format, ...) \
    kmt->spin_lock(&lock_print);\
    printf(format, ## __VA_ARGS__); \
    kmt->spin_unlock(&lock_print)
#define warning(format, ...) \
    lprintf("\033[33m" format "\033[0m", ## __VA_ARGS__);

#define true 1
#define false 0
#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))

extern int initialized;

#endif
