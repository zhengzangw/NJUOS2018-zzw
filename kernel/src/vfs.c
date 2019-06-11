#include <common.h>
#include <vfs.h>
#include <devices.h>
#include <kmt.h>

void vfs_init(){
    // ext2fs -> /
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
    vfs->mkdir("/dev");
    vfs->mkdir("/proc");
    vfs->mkdir("/etc");
    int fd = vfs->open("/etc/passwd", O_CREAT);
    const char *words = "zhengzangw:x:1000:1000:zhengzangw,,,:/home/zhengzangw:/bin/awsh";
    vfs->write(fd, words, strlen(words));
    vfs->close(fd);

    // ext2fs -> /mnt
    filesystem_t *fs2 = pmm->alloc(sizeof(filesystem_t));
    fs2->ops = &ext2_ops;
    fs2->dev = dev_lookup("ramdisk1");
    fs2->ops->init(fs2, "/", fs2->dev);
    vfs->mount("/mnt/", fs2);

    // devfs -> /dev
    filesystem_t *devfs = pmm->alloc(sizeof(filesystem_t));
    devfs->ops = &devfs_ops;
    devfs->dev = NULL;
    devfs->ops->init(devfs, "/", devfs->dev);
    vfs->mount("/dev/", devfs);

    // procfs -> /proc
    filesystem_t *procfs = pmm->alloc(sizeof(filesystem_t));
    procfs->ops = &procfs_ops;
    procfs->dev = NULL;
    procfs->ops->init(procfs, "/", procfs->dev);
    vfs->mount("/proc/", procfs);
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

#define checkfs(func) if (mpt[index].fs->ops->func==NULL) return -1
#define checkinode(func) if (FILE(fd)->inode->ops->func==NULL) return -1
int vfs_mkdir(const char *path){
    int index = get_mount(path);
    checkfs(mkdir);
    return mpt[index].fs->ops->mkdir(mpt[index].fs, RAW(path));
}

int vfs_rmdir(const char *path){
    int index = get_mount(path);
    if (strlen(RAW(path))<=1) return -1;
    checkfs(rmdir);
    return mpt[index].fs->ops->rmdir(mpt[index].fs, RAW(path));
}

int vfs_link(const char *oldpath, const char *newpath){
    int fd = vfs->open(oldpath, 0);
    int index = get_mount(newpath);
    checkinode(link);
    return FILE(fd)->inode->ops->link(FILE(fd), newpath+strlen(mpt[index].path));
}

int vfs_unlink(const char *path){
    int index = get_mount(path);
    checkfs(unlink);
    return mpt[index].fs->ops->unlink(mpt[index].fs, RAW(path));
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

spinlock_t lock_kmt;
int vfs_open(const char *path, int flags){
    int index = get_mount(path);
    inode_t* cur = mpt[index].fs->ops->lookup(mpt[index].fs, RAW(path), 0);
    if (cur == NULL) {
        if (flags & O_CREAT){
            checkfs(create);
            int ret = mpt[index].fs->ops->create(mpt[index].fs, RAW(path));
            if (ret==0) cur = mpt[index].fs->ops->lookup(mpt[index].fs, RAW(path), 0);
            else return -1;
            if (cur == NULL) return -1;
        } else {
            return -1;
        }
    }

    kmt->spin_lock(&lock_kmt);
    int findex = get_free_flides(_cpu());
    assert(findex>=0);
    //assert(cputask[_cpu()]);

    FILE(findex) = pmm->alloc(sizeof(file_t));
    FILE(findex)->inode = cur;
    file_t tmp = FILE(findex);
    kmt->spin_unlock(&lock_kmt);

    cur->ops->open(tmp, flags);

    return findex;
}

int vfs_close(int fd){
    if (FILE(fd)==NULL) return -1;
    checkinode(close);
    FILE(fd)->inode->ops->close(cputask[_cpu()]->flides[fd]);
    pmm->free(cputask[_cpu()]->flides[fd]);
    FILE(fd) = NULL;
    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    if (FILE(fd)==NULL) return -1;
    checkinode(read);
    return FILE(fd)->inode->ops->read(FILE(fd), (char *)buf, nbyte);
}

ssize_t vfs_write(int fd, const void *buf, size_t nbyte){
    if (FILE(fd)==NULL) return -1;
    checkinode(write);
    return FILE(fd)->inode->ops->write(FILE(fd), (char *)buf, nbyte);
}

off_t vfs_lseek(int fd, off_t offset, int whence){
    if (FILE(fd)==NULL) return -1;
    checkinode(lseek);
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
