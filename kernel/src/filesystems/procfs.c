#include <vfs.h>
#include <common.h>
#include <devices.h>
#include <kmt.h>

spinlock_t lock_kmt;

void procfs_init(filesystem_t *fs, const char *name, device_t *dev){
}

task_t *tasks[MAXTASK];
int task_num(){
    int ret = 0;
    kmt->spin_lock(&lock_kmt);
    for (int i=0;i<MAXTASK;++i){
        if (tasks[i]){
            ret ++;
        }
    }
    kmt->spin_unlock(&lock_kmt);
    return ret;
}

inode_t* procfs_lookup(filesystem_t *fs, const char *name, int flags){
    Log("name = %s",name);
    int finded = 0;
    inode_t *ret = balloc(sizeof(inode_t));
    if (strcmp(name, "")==0||strcmp(name, ".")==0||strcmp(name,"..")==0){
        finded = 1;
        ret->id = 0;
        ret->type = DR;
        ret->dir_len = 2+task_num();
        ret->fs_inode = NULL;
    } else if (strcmp(name, "cpuinfo")==0) {
        finded = 1;
        ret->id = 1;
        ret->type = NF;
        ret->dir_len = 0;
        ret->fs_inode = NULL;
    } else if (strcmp(name, "meminfo")==0) {
        finded = 1;
        ret->id = 2;
        ret->type = NF;
        ret->dir_len = 0;
        ret->fs_inode = NULL;
    } else if (isnum(name)) {
        Logint(atoi(name));
        if (tasks[atoi(name)]) {
            ret->id = 3;
            ret->type = NF;
            ret->dir_len = 0;
            ret->fs_inode = pmm->alloc(strlen(name)+1);
            strcpy(ret->fs_inode, name);
        }
    }
    if (finded){
        ret->fs = fs;
        ret->ops = &procfs_inodeops;
        ret->permission = R_OK;
        ret->size = 0;
        ret->link_num = 1;
        return ret;
    } else {
        pmm->free(ret);
        return NULL;
    }
}

ssize_t procfs_inode_read(file_t *file, char *buf, size_t size){
    return 0;
}

int procfs_inode_open(file_t *file, int flags){
    file->offset = 0;
    return 0;
}

int procfs_inode_close(file_t *file){
    return 0;
}

fsops_t procfs_ops = {
    .init = procfs_init,
    .lookup = procfs_lookup,
    .close = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .create = NULL,
};

inodeops_t procfs_inodeops = {
    .open = procfs_inode_open,
    .close = procfs_inode_close,
    .read = procfs_inode_read,
    .write = NULL,
    .link = NULL,
    .lseek = NULL,
};
