#include <common.h>
#include <vfs.h>
#include <devices.h>

void ext2_init(filesystem_t *fs, const char *name, device_t *dev){

}

inode_t ext2_lookup(filesystem_t *fs, const char *name, int flags){
    return NULL;
}

int close(inode_t *inode){
    return 0;
}

fsops_t ext2_ops = {
    .init = ext2_init,
    .lookup = ext2_lookup,
    .close = ext2_close,
};
