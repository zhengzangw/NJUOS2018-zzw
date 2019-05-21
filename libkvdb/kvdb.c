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
    return 0;
}

int kvdb_close(kvdb_t *db){
    fclose(db->file);
    return 0;
}

int kvdb_put(kvdb_t *db, const char *key, const char *value){
    fseek(db->file, 0, SEEK_END);
    fprintf(db->file, "\n%s %s\n", key, value);
    return 0;
}

#define backspace(file) \
    fseek(file, -1, SEEK_CUR)
#define fscanf_bak(file, c) \
    backspace(file); \
    fscanf(file, "%c", &c);\
    backspace(file)
#define fscanf_bak_0a(file, c) \
    fscanf_bak(file, c); \
    assert(flag=='\n');

char *kvdb_get(kvdb_t *db, const char *key){
    char flag;
    int finded = 0, len;
    fseek(db->file, 0, SEEK_END);

    while (!finded){
        fscanf_bak_0a(db->file, flag);

        len = 0;
        flag = '\0';
        while (flag!=' '){
            len++;
            fscanf_bak(db->file, flag);
        }
        printf("len=%d\n", len);
        assert(0);
        /*
        fseek(db->file, 2, SEEK_CUR);
        char* value = malloc(len);
        fscanf(db->file, "%s", value);
        fseek(db->file, -len, SEEK_CUR);

        fseek(db->file, -1, SEEK_CUR);
        fscanf_cb(db->file, flag);
        assert(flag=='\n');
        int len = 0;
        flag = '\0';
        while (flag!='\n'){
            len++;
            fscanf_cb(db->file, flag);
        }
        fseek(db->file, 2, SEEK_CUR);

        fseek(db->file, -1, SEEK_CUR);
        fscanf_cb(db->file, flag);
        assert(flag=='\n');
        */
    }
    return NULL;
}
