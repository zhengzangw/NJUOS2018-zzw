#include "kvdb.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#define kvdb_get_log(db, key)\
    value = kvdb_get(db, key);\
    if (value!=NULL) printf("[%s]: [%s]\n", key, value);\
    free(value)

kvdb_t db;
pthread_t threads[10];
typedef struct ds {
    char *key, *value;
} ds_t;
void *producer(void* args){
    ds_t *data = (ds_t *)args;
    while (1){
        kvdb_put(&db, data->key, data->value);
    }
}

void *consumer(void* key){
    char *kkey = (char *)key;
    char *value;
    while (1){
      kvdb_get_log(&db, kkey);
    }
}

int main(int argc, char *argv[]){
    char *value;
    kvdb_open(&db, "a.db");

    ds_t tmp;
    tmp.key = "zzw"; tmp.value = "171860658";
    pthread_create(&threads[0], NULL, producer, (void*)&tmp);

    pthread_create(&threads[5], NULL, consumer, (void*)"zzw");

    void *tptr;
    pthread_join(threads[0], &tptr);
    pthread_join(threads[5], &tptr);

    kvdb_close(&db);
    return 0;
}
