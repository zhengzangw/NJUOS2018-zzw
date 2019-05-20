#ifndef __KVDB_H__
#define __KVDB_H__

#include <stdio.h>
#include <stdint.h>

struct kvdb_header {
  char ind[4];
  uint16_t free_ptr;
  uint16_t journal_ptr;
}__attribute__((packed));
typedef struct kvdb_header kvdb_header_t;

enum Type { KEY_SHORT, KEY_LONG, VALUE_SHORT, VALUE_LONG };
enum Status { FREE, FULL };
struct entry {
  uint8_t type;
  uint8_t status;
  uint16_t ptr_next;
  uint16_t ptr_pair;
  uint8_t data[122];
}__attribute__((packed));
typedef struct entry entry_t;

struct kvdb {
  FILE* file;
  kvdb_header_t* info;
};
typedef struct kvdb kvdb_t;

int kvdb_open(kvdb_t *db, const char *filename);
int kvdb_close(kvdb_t *db);
int kvdb_put(kvdb_t *db, const char *key, const char *value);
char *kvdb_get(kvdb_t *db, const char *key);

#endif
