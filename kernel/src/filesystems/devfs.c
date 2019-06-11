#include <common.h>
#include <vfs.h>
#include <devices.h>

void devfs_init(filesystem_t *fs, const char *name, device_t *dev){
}
inode_t* devfs_lookup(filesystem_t *fs, const char *name, int flags){
    int finded = 0;
    inode_t *ret = balloc(sizeof(inode_t));
    if (strcmp(name,"/")){
      finded = 1;
      ret->id = 0;
      ret->fs_inode = NULL;
      ret->type = DR;
      ret->dir_len = 6;
    } else {
      device_t *tmp = dev_lookup(name);
      if (tmp) {
          finded = 1;
          ret->id = tmp->id;
          ret->fs_inode = tmp;
          ret->type = DV;
          ret->dir_len = 0;
      }
    }
    if (!finded) {
        pmm->free(ret);
        return NULL;
    }
    else {
        ret->fs = fs;
        ret->ops = &devfs_inodeops;
        ret->permission = R_OK|W_OK;
        ret->size = 0;
        ret->link_num = 1;
        return ret;
    }
}

int devfs_inode_open(file_t *file, int flags){
    file->offset = 0;
    return 0;
}
int devfs_inode_close(file_t *file){
    return 0;
}

#define DEV(file) ((device_t*)file->inode->fs_inode)
const char *devfs_ls ="tty1\ntty2\ntty3\ntty4\nramdisk0\nramdisk1\n";
ssize_t devfs_inode_read(file_t *file, char *buf, size_t size){
    ssize_t ret;
    if (file->inode->id==0){
        strcpy(buf, devfs_ls);
        ret = strlen(devfs_ls);
    } else {
        ret = DEV(file)->ops->read(DEV(file), file->offset, buf, size);
    }
    return ret;
}
ssize_t devfs_inode_write(file_t *file, const char *buf, size_t size){
    ssize_t ret;
    if (file->inode->id==0){
        ret = -1;
    } else {
        ret = DEV(file)->ops->write(DEV(file), file->offset, buf, size);
    }
    return ret;
}

fsops_t devfs_ops = {
    .init = devfs_init,
    .lookup = devfs_lookup,
    .close = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .create = NULL,
};

inodeops_t devfs_inodeops = {
    .open = devfs_inode_open,
    .close = devfs_inode_close,
    .read = devfs_inode_read,
    .write = devfs_inode_write,
    .link = NULL,
    .lseek = NULL,
};
