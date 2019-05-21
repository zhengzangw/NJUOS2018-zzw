#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

#include <assert.h>
#define backspace(file, num) \
    fseek(file, num, SEEK_CUR)
#define fscanf_bak(file, c) \
    fscanf(file, "%c", &c);\
    backspace(file, -2)
#define fscanf_bak_0a(file, c) \
    fscanf_bak(file, c); \
    assert(c=='\n')
#define fscanf_val(file, des, len)\
    des = malloc(len);\
    fseek(file, 1, SEEK_CUR);\
    fscanf(file, " %s", des);\
    fseek(file, -len-1, SEEK_CUR)

static inline bool ishead(kvdb_t *db){
    printf("ftell=%ld\n", ftell(db->file));
    return ftell(db->file)==0;
}

char *kvdb_get(kvdb_t *db, const char *key){
    char flag;
    char *tmp_value, *tmp_key;
    bool finded = 0, error = 0;
    int len;

    file_lock_sh(db, NULL);
    fseek(db->file, -1, SEEK_END);
    while (!finded && !ishead(db)){
        //Check the last \n
        fscanf_bak_0a(db->file, flag);
        //Value
        len = 0; flag = '\0';
        while (flag!=' ' && !ishead(db)){
            len++;
            fscanf_bak(db->file, flag);
            assert(flag!='\n');
        }
        assert(flag==' ');
        fscanf_val(db->file, tmp_value, len);
        //Key
        len = 0; flag = '\0';
        while (flag!='\n' && !ishead(db)){
            len++;
            fscanf_bak(db->file, flag);
            assert(flag!=' ');
        }
        if (ishead(db)) {
            len++;
            ishead(db);
            fscanf_bak(db->file, flag);
            ishead(db);
        }
        assert(flag=='\n');
        fscanf_val(db->file, tmp_key, len);
        //Check
        if (strcmp(tmp_key, key)==0) {
            finded = 1;
        } else {
            printf("%s\n", tmp_key);
            free(tmp_value);
        }
        free(tmp_key);
    }
    file_lock_un(db, NULL);

    if (finded){
        return tmp_value;
    } else {
        if (error) {
            fprintf(stderr, "Date Base Not Consistent\n");
        } else {
            fprintf(stderr, "key [%s] not founded\n", key);
        }
        return NULL;
    }
}
