#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

int kvdb_open(kvdb_t *db, const char *filename){
    // Judge
    if (access(filename, F_OK)==0){
        if (access(filename, R_OK)!=0 || access(filename, W_OK)!=0){
            return -1;
        }
        db->fd = open(filename, O_RDWR);
        read(db->fd, db->info, sizeof(kvdb_header_t));
    } else {
        db->fd = open(filename, O_RDWR|O_CREAT);
        db->info = malloc(sizeof(kvdb_header_t));
        strcpy(db->info->ind, "MDB");
        db->info->free_ptr = 1;
        write(db->fd, db->info, sizeof(kvdb_header_t));
    }
    printf("%s\n", db->info->ind);
    assert(strcmp(db->info->ind, "MDB")==0);
    return -1;
}
int kvdb_close(kvdb_t *db){
    close(db->fd);
    return -1;
}
int kvdb_put(kvdb_t *db, const char *key, const char *value){
    return -1;
}
char *kvdb_get(kvdb_t *db, const char *key){
    /*
    char *tmp_key = malloc(128);
    char *tmp_value = malloc(128);
    while (scanf("%s", tmp_key)!=EOF){
        scanf("%s", tmp_value);
        if (strcmp(tmp_key, key)==0){
            break;
        }
    }
    free(tmp_key);
    return tmp_value;
    */
    return NULL;
}
