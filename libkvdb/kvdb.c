#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/file.h>
#include <pthread.h>

#define check_file_lock(ret,fail) do { \
    if (ret==-1) { \
        perror("Cannot set file lock"); \
        return fail; \
    } } while (0)

#define file_lock_ex(db,fail) \
    check_file_lock(ret,fail)
#define file_lock_sh(db,fail) \
    ret = flock(fileno(db->file), LOCK_SH);\
    check_file_lock(ret,fail)
#define file_lock_un(db,fail) \
    ret = flock(fileno(db->file), LOCK_UN);\
    check_file_lock(ret,fail)

#define assert(exp)\
    do {if (!(exp)){\
        error = 1;\
        break;\
    }}while (0)
#define backspace(file, num) \
    fseek(file, num, SEEK_CUR)
#define fscanf_bak(file, c) \
    backspace(file, -1);\
    fscanf(file, "%c", &c);\
    backspace(file, -1);
#define fscanf_bak_0a(file, c) \
    fscanf_bak(file, c); \
    assert(c=='\n')
#define fscanf_val(file, des, len)\
    des = malloc(len);\
    fscanf(file, "%s", des);\
    fseek(file, -len, SEEK_CUR)

static inline bool ishead(kvdb_t *db){
    return ftell(db->file)==0;
}

int check_journal(kvdb_t *db){
    char flag;
    bool consist = 0, has_blank = 0;

    fseek(db->file, -1, SEEK_END);
    while (!consist && !ishead(db)){
        fscanf_bak(db->file, flag);
        if (flag=='\n'){
            has_blank = 0;
            flag = '\0';
            while (flag!='\n'){
                fscanf_bak(db->file, flag);
                if (flag == ' ') has_blank = 1;
            }
            if (flag == '\n' && has_blank == 1) consist = 1;
        }
    }
    return 0;
}

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

    if (pthread_mutex_init(&db->mutex, NULL) != 0){
        perror("Cannot create lock");
        return -1;
    }

    db->file = fopen(filename, "r+");
    if (db->file == NULL){
        perror("Cannot open file");
        return -1;
    }

    return 0;
}

int kvdb_close(kvdb_t *db){
    pthread_mutex_destroy(&db->mutex);
    if (db->file==NULL) return -1;
    fclose(db->file);
    return 0;
}

int kvdb_put(kvdb_t *db, const char *key, const char *value){
    if (db->file==NULL) return -1;
    if (pthread_mutex_lock(&db->mutex)!=0) return -1;
    if (flock(fileno(db->file), LOCK_EX) !=0) return -1;
    fseek(db->file, 0, SEEK_END);
    fprintf(db->file, "\n%s %s\n", key, value);
    fsync(fileno(db->file));
    if (flock(fileno(db->file), LOCK_UN) !=0) return -1;
    if (pthread_mutex_unlock(&db->mutex)) return -1;
    return 0;
}

char *kvdb_get(kvdb_t *db, const char *key){
    char flag;
    char *tmp_value, *tmp_key;
    bool finded = 0, error = 0;
    int len;

    if (db->file==NULL) return NULL;
    if (pthread_mutex_lock(&db->mutex)!=0) return NULL;
    if (flock(fileno(db->file), LOCK_SH) !=0) return NULL;
    fseek(db->file, 0, SEEK_END);
    while (!finded && !ishead(db)){
        //Check the last \n
        fscanf_bak_0a(db->file, flag);
        //Value
        len = 0; flag = '\0';
        while (flag!=' ' && !ishead(db)){
            len++;
            fscanf_bak(db->file, flag);
        }
        assert(flag==' ');
        fscanf_val(db->file, tmp_value, len);
        //Key
        len = 0; flag = '\0';
        while (flag!='\n' && !ishead(db)){
            len++;
            fscanf_bak(db->file, flag);
        }
        assert(flag=='\n');
        fscanf_val(db->file, tmp_key, len);
        //Check
        if (strcmp(tmp_key, key)==0) {
            finded = 1;
        } else {
            free(tmp_value);
        }
        free(tmp_key);
    }
    if (flock(fileno(db->file), LOCK_UN) !=0) return NULL;
    pthread_mutex_unlock(&db->mutex);

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
