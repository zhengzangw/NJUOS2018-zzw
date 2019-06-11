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
    if (!path) return NULL;
    char *ret = NULL;
    for (int i=0;i<strlen(path);++i){
        if (path[i] == '/'){
            ret = balloc(i+2);
            strncpy(ret, path, i);
            break;
        }
    }
    if (ret==NULL) strcpy(ret, path);
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> zsh
//path = usr -> usr
char *filename(const char* path){
    if (!path) return NULL;
    char *ret = NULL;
    if (strlen(path)==0) return ret;
    for (int i=strlen(path);i>0;--i){
        if (path[i] == '/'){
            ret = balloc(strlen(path-i+1)+1);
            strncpy(ret, path+i+1, strlen(path-i+1));
            break;
        }
    }
    if (ret==NULL) strcpy(ret, path);
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> usr/bin
//path = usr -> NULL
char *alldir(const char *path){
    if (!path) return NULL;
    char *ret = NULL;
    if (strlen(path)==0) return ret;
    for (int i=strlen(path);i>0;--i){
        if (path[i] == '/'){
            ret = balloc(i+2);
            strncpy(ret, path, i);
            break;
        }
    }
    if (ret==NULL) strcpy(ret, path);
    Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> bin/zsh
//path = usr -> NULL
char *postname(const char *path){
    if (!path) return NULL;
    char *ret = NULL;
    if (strlen(path)==0) return ret;
    for (int i=strlen(path);i>0;--i){
        if (path[i] == '/'){
            ret = balloc(strlen(path-i+1)+1);
            strncpy(ret, path+i+1, strlen(path-i+1));
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