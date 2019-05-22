#include "kvdb.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define kvdb_get_log(db, key)\
    value = kvdb_get(db, key);\
    if (value!=NULL) printf("[%s]: [%s]\n", key, value);\
    free(value)

int main(int argc, char *argv[]){
    kvdb_t db;
    char *value;

    kvdb_open(&db, "a.db");
    //kvdb_put(&db, "zzw", "awsl");

    kvdb_get_log(&db, "zzw");
    assert(0);
    while (1){
      kvdb_put(&db, "zzw", argv[1]);
      kvdb_get_log(&db, "zzw");
      assert(value!=NULL);
    }

    kvdb_close(&db);
    return 0;
}
