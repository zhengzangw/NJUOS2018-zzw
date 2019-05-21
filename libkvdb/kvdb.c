#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

int kvdb_open(kvdb_t *db, const char *filename){
    if (access(filename, F_OK)==0){
        if (access(filename, R_OK)!=0 || access(filename, W_OK)!=0){
            return -1;
        }
    }
    db->file = fopen(filename, "w+");
    return -1;
}
int kvdb_close(kvdb_t *db){
    fclose(db->file);
    return -1;
}
int kvdb_put(kvdb_t *db, const char *key, const char *value){
    fseek(db->file, 0, SEEK_END);
    printf("\n%s %s\n", key, value);
    return -1;
}
char *kvdb_get(kvdb_t *db, const char *key){
    return NULL;
}
