#include <common.h>
#include <vfs.h>
#include <devices.h>

#define INODE_SIZE 64
struct ext2_inode {
  unsigned char exists : 1;
  unsigned char type : 3;
  unsigned char permission :3;
  unsigned char none : 1;
  int len;
  int link[62];
}__attribute__((packed));
typedef ext2_inode ext2_inode_t;
enum TYPE {NF, DR, LK, MP};

#define BLOCK_SIZE (1<<10)
#define BLOCK(x) (x)*BLOCK_SIZE
#define BLOCK_NUM (1<<7)
#define inode_table(i) BLOCK(0)+(i)*INODE_SIZE
#define data(i) BLOCK(16)+(i)*BLOCK_SIZE

int get_free_data(filesystem_t *fs, device_t *dev){
    int tmp = 1;
    for (int i=0;i<BLOCK_NUM;++i){
        dev->ops->read(dev, data(i), &tmp, sizeof(int));
        if (tmp==0) {
            tmp = 1;
            dev-ops-write(dev, data(i), &tmp, sizeof(int));
            return i;
        }
    }
    return -1;
}

void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    printf("size=%ld", sizeof(ext2_inode));
    ext2_inode_t *root = pmm->malloc(sizeof(ext2_inode_t));
    memset(root, 0, sizeof(root));
    root->exists = 1;
    root->type = DR;
    root->permission = R_OK|W_OK|X_OK;
    root->none = 0;
    root->len = 1;
    root->link[0] = get_free_data(fs, dev);

    dev->ops->write(dev, inode_table(0), &root, INODE_SIZE);
}

inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags){
    return NULL;
}

int ext2_close(inode_t *inode){
    return 0;
}

fsops_t ext2_ops = {
    .init = ext2_init,
    .lookup = ext2_lookup,
    .close = ext2_close,
};
