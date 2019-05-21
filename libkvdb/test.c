#include "kvdb.h"
#include <stdlib.h>

#define kvdb_get_log(db, key)\
    value = kvdb_get(db, key);\
    if (value!=NULL) printf("[%s]: [%s]\n", key, value);\
    free(value)

int main(int argc, char *argv[]){
    kvdb_t db;
    char *value;

    kvdb_open(&db, "a.db");

    while (1){
      kvdb_put(&db, "zzw", argv[1]);
      kvdb_put(&db, "jyy", "ree-easy-pieces");
      kvdb_put(&db, "wrong", "e-easy-pieces");
      kvdb_get_log(&db, "zzw");
    }

    kvdb_close(&db);
    return 0;
}
