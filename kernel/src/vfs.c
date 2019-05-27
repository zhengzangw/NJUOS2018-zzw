#include <common.h>
#include <vfs.h>
#include <devices.h>

extern fsops_t ext2_ops;

void vfs_init(){
    filesystem_t fs = {
        .ops = &ext2_ops,
        .dev = dev_lookup("ramdisk0"),
    };
    vfs->mount("/", &fs);
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
    strcpy(mpt[index], path);
    mpt[index].fs = fs;
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

int get_fs(const char *path){
    int index;
    for (int i=0;i<MAXMP;++i){
        if (strncmp(path, mpt[i].path, strlen(mpt[i].path))==0){
            index = i;
            break;
        }
    }
    return index;
}

int vfs_access(const char *path, int mode){
    int index = get_fs(path);
    inode_t* cur = mpt[index].fs->fsops->lookup(mpt[i], name, 0);
    if ((mode|F_OK)&&!cur) return -1;
    if ((mode|F_OK)&&(cur->permission|R_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|W_OK)) return -1;
    if ((mode|F_OK)&&(cur->permission|X_OK)) return -1;
    return 0;
}


int vfs_mkdir(const char *path){
    int index = get_fs(path);
    inode_t* cur = mpt[index].fs->fsops->lookup(mpt[i], name, 0);
    cur->mkdir(path);
    return 0;
}

int vfs_rmdir(const char *path){
    int index = get_fs(path);
    inode_t* cur = mpt[index].fs->fsops->lookup(mpt[i], name, 0);
    cur->rmdir(path);
    return 0;
}

int vfs_link(const char *oldpath, const char *newpath){
    int index = get_fs(path);
    inode_t* cur = mpt[index].fs->fsops->lookup(mpt[i], name, 0);
    cur->link(path, cur);
    return 0;
}

int vfs_unlink(const char *path){
    int index = get_fs(path);
    inode_t* cur = mpt[index].fs->fsops->lookup(mpt[i], name, 0);
    cur->unlink(path);
    return 0;
}

int vfs_open(const char *path, int flags){
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
