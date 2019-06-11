#include <common.h>
#include <vfs.h>
#include <devices.h>
#include <kmt.h>

void vfs_init(){
    // Load / as ext2 filesystem
    filesystem_t *fs = pmm->alloc(sizeof(filesystem_t));
    fs->ops = &ext2_ops;
    fs->dev = dev_lookup("ramdisk0");
    fs->ops->init(fs, "/", fs->dev);
    vfs->mount("/", fs);

    vfs->mkdir("/bin");
    vfs->mkdir("/home");
    vfs->mkdir("/usr");
    vfs->mkdir("/usr/bin");
    vfs->mkdir("/mnt");
    vfs->mkdir("/etc");
    int fd = vfs->open("/etc/passwd", O_CREAT);
    const char *words = "zhengzangw:x:1000:1000:zhengzangw,,,:/home/zhengzangw:/bin/awsh";
    vfs->write(fd, words, strlen(words));
    vfs->close(fd);

    // Load /mnt
    filesystem_t *fs2 = pmm->alloc(sizeof(filesystem_t));
    fs2->ops = &ext2_ops;
    fs2->dev = dev_lookup("ramdisk1");
    fs2->ops->init(fs2, "/", fs2->dev);
    vfs->mount("/mnt/", fs2);
}

mountpoint_t mpt[MAXMP];
int vfs_mount(const char *path, filesystem_t *fs){
    int index = -1;
    for (int i=0;i<MAXMP;++i){
        if (index==-1 && !mpt[i].exists){
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

static int get_mount(const char *path){
    int index=-1, len = 0;
    for (int i=0;i<MAXMP;++i){
        if (mpt[i].exists && strncmp(path, mpt[i].path, strlen(mpt[i].path))==0){
            if (len<strlen(mpt[i].path)){
                len = strlen(mpt[i].path);
                index = i;
            }
        }
    }
    
    return index;
}

#define RAW(path) (path+strlen(mpt[index].path))
int vfs_access(const char *path, int mode){
    int index = get_mount(path);
    inode_t* cur = mpt[index].fs->ops->lookup(mpt[index].fs, path, 0);

    if ((mode|F_OK)&&!cur) return -1;
    if ((mode|F_OK)&&(cur->permission|R_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|W_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|X_OK)) return -1;

    pmm->free(cur);
    return 0;
}


int vfs_mkdir(const char *path){
    int index = get_mount(path);
    return mpt[index].fs->ops->mkdir(mpt[index].fs, RAW(path));
}

int vfs_rmdir(const char *path){
    int index = get_mount(path);
    return mpt[index].fs->ops->rmdir(mpt[index].fs, RAW(path));
}

int vfs_link(const char *oldpath, const char *newpath){
    int fd = vfs->open(oldpath, 0);
    int index = get_mount(newpath);
    return FILE(fd)->inode->ops->link(FILE(fd), newpath+strlen(mpt[index].path));
}

int vfs_unlink(const char *path){
    int index = get_mount(path);
    return mpt[index].fs->ops->unlink(mpt[index].fs, path);
}

int initialized;
int get_free_flides(int ccppuu){
    if (initialized == 0) return 0;
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
    inode_t* cur = mpt[index].fs->ops->lookup(mpt[index].fs, RAW(path), 0);
    if (cur == NULL) {
        if (flags & O_CREAT){
            int ret = mpt[index].fs->ops->create(mpt[index].fs, RAW(path));
            if (ret==0) cur = mpt[index].fs->ops->lookup(mpt[index].fs, RAW(path), 0);
            else return -1;
            if (cur == NULL) return -1;
        } else {
            return -1;
        }
    }

    int findex = get_free_flides(_cpu());
    assert(findex>=0);
    //assert(cputask[_cpu()]);

    cputask[_cpu()]->flides[findex] = pmm->alloc(sizeof(file_t));
    cputask[_cpu()]->flides[findex]->inode = cur;

    cur->ops->open(cputask[_cpu()]->flides[findex], flags);

    return findex;
}

int vfs_close(int fd){
    cputask[_cpu()]->flides[fd]->inode->ops->close(cputask[_cpu()]->flides[fd]);
    pmm->free(cputask[_cpu()]->flides[fd]);
    cputask[_cpu()]->flides[fd] = NULL;
    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    inode_t* tmp = cputask[_cpu()]->flides[fd]->inode;
    return tmp->ops->read(cputask[_cpu()]->flides[fd], (char *)buf, nbyte);
}

ssize_t vfs_write(int fd, const void *buf, size_t nbyte){
    inode_t* tmp = FILE(fd)->inode;
    return tmp->ops->write(FILE(fd), (char *)buf, nbyte);
}

off_t vfs_lseek(int fd, off_t offset, int whence){
    return FILE(fd)->inode->ops->lseek(FILE(fd), offset, whence);
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
