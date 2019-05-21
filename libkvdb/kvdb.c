#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/file.h>

#define check_file_lock(ret,fail) do { \
    if (ret==-1) { \
        perror("Cannot set file lock"); \
        return fail; \
    } } while (0)

#define file_lock_ex(db,fail) \
    ret = flock(fileno(db->file), LOCK_EX);\
    check_file_lock(ret,fail)
#define file_lock_sh(db,fail) \
    ret = flock(fileno(db->file), LOCK_SH);\
    check_file_lock(ret,fail)
#define file_lock_un(db,fail) \
    ret = flock(fileno(db->file), LOCK_UN);\
    check_file_lock(ret,fail)

int ret;
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
    fclose(db->file);
    return 0;
}

int kvdb_put(kvdb_t *db, const char *key, const char *value){
    file_lock_ex(db, -1);
    fseek(db->file, 0, SEEK_END);
    fprintf(db->file, "\n%s %s\n", key, value);
    file_lock_un(db, -1);
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
    if (c!='\n'){\
        printf("flag = %c\n", flag);\
    }\
    assert(c=='\n')

static inline bool ishead(kvdb_t *db){
    return ftell(db->file)==0;
}

char *kvdb_get(kvdb_t *db, const char *key){
    char flag;
    char *tmp_value, *tmp_key;
    int finded = 0, len;

    file_lock_sh(db, NULL);
    fseek(db->file, 0, SEEK_END);

    while (!finded && !ishead(db)){
        puts("1");
        fscanf_bak_0a(db->file, flag);
        puts("1");

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
        puts("1");

    file_lock_un(db, NULL);

    if (finded){
        return tmp_value;
    } else {
        fprintf(stderr, "key [%s] not founded\n", key);
        return NULL;
    }
}
