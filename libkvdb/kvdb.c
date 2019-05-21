#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/file.h>

#define file_lock_ex(db) do {\
    int ret = flock(fileno(db->file), LOCK_EX);\
    if (ret==-1) { \
        perror("Cannot set file lock"); \
        return -1; \
    } } while (0)\

int kvdb_open(kvdb_t *db, const char *filename){
    if (access(filename, F_OK)==0){
        if (access(filename, R_OK)!=0 || access(filename, W_OK)!=0){
            perror("Permissions not enough");
            return -1;
        }
    } else {
        FILE* tmpf = fopen(filename, "w+");
        if (tmpf == NULL){
            perror("Cannot create file");
            return -1;
        }
        fclose(tmpf);
    }

    db->file = fopen(filename, "r+");
    if (db->file == NULL){
        perror("Cannot open file");
        return -1;
    }


    return 0;
}

int kvdb_close(kvdb_t *db){
    file_lock_ex(db);
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

static inline bool ishead(kvdb_t *db){
    return ftell(db->file)==0;
}

char *kvdb_get(kvdb_t *db, const char *key){
    char flag;
    char *tmp_value, *tmp_key;
    int finded = 0, len;
    fseek(db->file, 0, SEEK_END);

    while (!finded && !ishead(db)){
        fscanf_bak_0a(db->file, flag);

        len = 0;
        flag = '\0';
        while (flag!=' '){
            len++;
            fscanf_bak(db->file, flag);
        }

        tmp_value = malloc(len);
        fscanf(db->file, "%s", tmp_value);
        fseek(db->file, -len+1, SEEK_CUR);

        len = 0;
        flag = '\0';
        while (flag!='\n'){
            len++;
            fscanf_bak(db->file, flag);
        }

        tmp_key = malloc(len);
        fscanf(db->file, "%s", tmp_key);
        fseek(db->file, -len+1, SEEK_CUR);

        if (strcmp(tmp_key, key)==0) {
            finded = 1;
        } else {
            free(tmp_value);
        }
        free(tmp_key);
    }

    if (finded){
        return tmp_value;
    } else {
        fprintf(stderr, "key [%s] not founded\n", key);
        return NULL;
    }
}
