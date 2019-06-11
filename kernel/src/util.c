#include <util.h>
#include <common.h>

char *rootdir(const char* path){
    char *ret = NULL;
    for (int i=0;i<strlen(path);++i){
        if (path[i] == '/'){
            ret = balloc(i+2);
            break;
        }
    }
    return ret;
}

// 1 if splited, 0 if done
// /bin/test/a -> / + bin/test/a
int split(const char *path, char **pre, char **post){
    int ret = 0, len = strlen(path);
    for (int i=0;i<len;++i){
        if (path[i]=='/'){
            ret = 1;
            *pre = balloc(i+2);
            strncpy(*pre, path, i+1);
            (*pre)[i+1] = '\0';
            *post = balloc(len-i+2);
            strncpy(*post, path+i+1, len-i+1);
            break;
        }
    }
    if (strlen(*post)==0) ret = 0;
    return ret;
}
// /bin/test/a -> / + /bin/test/
int split2(const char *path, char **pre, char **post){
    int ret = 0, len = strlen(path);
    for (int i=len-1;i>=0;--i){
        if (path[i]=='/'){
            ret = 1;
            *pre = pmm->alloc(i+2);
            strncpy(*pre, path, i+1);
            (*pre)[i+1] = '\0';
            *post = pmm->alloc(len-i+2);
            strncpy(*post, path+i+1, len-i+1);
            break;
        }
    }
    return ret;
}