#ifndef __LOCK_H__
#define __LOCK_H__

typedef struct __lock_t {int flag;} lock_t;

void init(lock_t *mutex);
void lock(lock_t *mutex);
void unlock(lock_t *mutex);

#endif