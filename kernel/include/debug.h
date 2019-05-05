#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <klib.h>

#define Log(format, ...) \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
          __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Logint(x) \
    Log(#x " = %d", x)

#define Assert(cond, ...) \
    do { \
        if (!(cond)) { \
            Log(__VA_ARGS__); \
            assert(cond); \
            } \
       } while (0)

#define panic(format, ...) \
    Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")"]]")
#endif
