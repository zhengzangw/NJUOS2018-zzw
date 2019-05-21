#include "kvdb.h"
#include <stdlib.h>

#define kvdb_get_log(db, key)\
    value = kvdb_get(db, key);\
    if (value!=NULL) printf("[%s]: [%s]\n", key, value);\
    free(value)

int main(){
    kvdb_t db;
    char *value;

    kvdb_open(&db, "a.db");
    kvdb_put(&db, "zzw", "171860658");
    kvdb_put(&db, "jyy", "ree-easy-pieces");
    kvdb_put(&db, "wrong", "e-easy-pieces");

    //while (1){
    kvdb_get_log(&db, "zzw");
    kvdb_get_log(&db, "wrong");
    kvdb_get_log(&db, "jyy");
    kvdb_get_log(&db, "problem");
    kvdb_get_log(&db, "zzw");
    //}

    kvdb_close(&db);
    return 0;
}
