#include <common.h>
#include <vfs.h>
#include <devices.h>
#include <kmt.h>

/* ======== Util ========*/
void *balloc(int size){
    void *ret = pmm->alloc(size+1);
    ret = memset(ret, 0, size);
    return ret;
}

// 1 if splited, 0 if done
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

extern fsops_t ext2_ops;
void vfs_init(){
    // Load / as ext2 filesystem
    filesystem_t *fs = pmm->alloc(sizeof(filesystem_t));
    fs->ops = &ext2_ops;
    fs->dev = dev_lookup("ramdisk0");

    fs->ops->init(fs, "/", fs->dev);
    vfs->mount("/", fs);
}

mountpoint_t mpt[MAXMP];
int vfs_mount(const char *path, filesystem_t *fs){
    int index = -1;
    for (int i=0;i<MAXMP;++i){
        if (!mpt[i].exists){
            index = i;
        } else {
            if (mpt[i].exists&&strcmp(path, mpt[i].path)==0){
                return -1;
            }
        }
    }
    mpt[index] = (mountpoint_t){
        .path = path,
        .fs = fs,
        .exists = 1,
    };
    return 0;
}

int vfs_unmount(const char *path){
    int finded = -1;
    for (int i=0;i<MAXMP;++i){
        if (mpt[i].exists&&strcmp(path, mpt[i].path)==0){
            mpt[i].exists = 0;
            finded = 0;
        }
    }
    return finded;
}

int get_mount(const char *path){
    int index=-1, len = 0;
    for (int i=0;i<MAXMP;++i){
        if (strncmp(path, mpt[i].path, strlen(mpt[i].path))==0){
            if (len<strlen(mpt[i].path)){
                len = strlen(mpt[i].path);
                index = i;
            }
        }
    }
    return index;
}

int vfs_access(const char *path, int mode){
    /*if ((mode|F_OK)&&!cur) return -1;
    if ((mode|F_OK)&&(cur->permission|R_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|W_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|X_OK)) return -1;
    */
    return 0;
}


int vfs_mkdir(const char *path){
    return 0;
}

int vfs_rmdir(const char *path){
    return 0;
}

int vfs_link(const char *oldpath, const char *newpath){
    return 0;
}

int vfs_unlink(const char *path){
    return 0;
}

int get_free_flides(int ccppuu){
    int index = -1;
    for (int i=0;i<NOFILE;++i){
        if (cputask[ccppuu]->flides[i]==NULL){
            index = i;
            break;
        }
    }
    return index;
}
int vfs_open(const char *path, int flags){
    int index = get_mount(path);
    inode_t* cur = mpt[index].fs->ops->lookup(mpt[index].fs, path, 0);

    int findex = get_free_flides(_cpu());
    assert(findex>=0);
    cputask[_cpu()]->flides[findex] = pmm->alloc(sizeof(file_t));

    cur->ops->open(cputask[_cpu()]->flides[findex], flags);

    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    return 0;
}

ssize_t vfs_write(int fd, void *buf, size_t nbyte){
    return 0;
}

off_t vfs_lseek(int fd, off_t offset, int whence){
    return 0;
}

int vfs_close(int fd){
    return 0;
}

MODULE_DEF(vfs){
    .init = vfs_init,
    .access = vfs_access,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .mkdir = vfs_mkdir,
    .rmdir = vfs_rmdir,
    .link = vfs_link,
    .unlink = vfs_unlink,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .lseek = vfs_lseek,
    .close = vfs_close,
};
