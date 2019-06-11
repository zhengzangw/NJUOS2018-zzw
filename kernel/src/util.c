#include <util.h>
#include <common.h>

void strip(char *tmp){
    int len = strlen(tmp);
    if (len!=1 && tmp[len-1]=='/'){
        tmp[len-1] = '\0';
    }
}

//path = usr/bin/zsh -> usr
//path = usr -> usr
char *rootdir(const char* path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path)+1);
    strcpy(ret, path);
    for (int i=0;i<strlen(path);++i){
        if (path[i] == '/'){
            strncpy(ret, path, i);
            ret[i] = '\0';
            break;
        }
    }
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> zsh
//path = usr -> usr
char *filename(const char* path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path)+1);
    strcpy(ret, path);
    for (int i=strlen(path);i>0;--i){
        if (path[i] == '/'){
            strncpy(ret, path+i+1, strlen(path+i+1));
            ret[strlen(path+i+1)+1] = '\0';
            break;
        }
    }
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> usr/bin
//path = usr -> NULL
char *alldir(const char *path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path));
    for (int i=strlen(path);i>=0;--i){
        if (path[i] == '/'){
            strncpy(ret, path, i);
            ret[i] = '\0';
            break;
        }
    }
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> bin/zsh
//path = usr -> NULL
char *postname(const char *path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path));
    for (int i=strlen(path);i>=0;--i){
        if (path[i] == '/'){
            strncpy(ret, path+i+1, strlen(path+i+1));
            ret[strlen(path+i+1)+1] = '\0';
            break;
        }
    }
    Log("path = %s, ret = %s", path, ret);
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