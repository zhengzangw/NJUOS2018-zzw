#include <am.h>
#include <amdev.h>

#define SIDE 16

static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}

#define HEART1 {\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,\
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,\
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,\
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,\
    W, W, R, R, R, R, R, R, R, R, W, W, R, R, R, R, W, W, B, B,\
    W, W, R, R, R, R, R, R, R, R, W, W, R, R, R, R, W, W, B, B,\
    W, W, R, R, R, R, R, R, R, R, R, R, R, R, R, R, W, W, B, B,\
    W, W, R, R, R, R, R, R, R, R, R, R, R, R, R, R, W, W, B, B,\
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,\
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,\
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,\
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,\
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B\
    }
#define HEART2 {\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,\
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,\
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,\
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,\
    W, W, R, R, W, W, W, W, R, R, R, R, R, R, R, R, W, W, B, B,\
    W, W, R, R, W, W, W, W, R, R, R, R, R, R, R, R, W, W, B, B,\
    W, W, R, R, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B,\
    W, W, R, R, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B,\
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,\
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,\
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,\
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,\
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,\
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B\
    }

#define Log(format, ...) \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Assert(cond, ...) \
  do { \
    if (!(cond)) { \
      Log(__VA_ARGS__); \
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) \
  Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")
