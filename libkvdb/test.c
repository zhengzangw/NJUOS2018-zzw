#include "kvdb.h"
#include <stdlib.h>

int main(){
    kvdb_t db;
    const char *key = "operating-systems";
    char *value;

    kvdb_open(&db, "a.db");
    kvdb_put(&db, key, "three-easy-pieces");
    kvdb_put(&db, key, "ree-easy-pieces");
    kvdb_put(&db, key, "e-easy-pieces");
    value = kvdb_get(&db, key);
    kvdb_close(&db);
    printf("[%s]: [%s]\n", key, value);
    free(value);
    return 0;
}
