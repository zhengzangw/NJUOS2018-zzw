#include <common.h>
#include <vfs.h>
#include <devices.h>

#define INODE_SIZE 64
struct ext2_inode {
  int exists;
  unsigned short type;
  unsigned short permission;
  int len;
  int link[60];
}__attribute__((packed));
typedef struct ext2_inode ext2_inode_t;
enum TYPE {NF, DR, LK, MP};

#define BLOCK_SIZE (1<<10)
#define BLOCK(x) (x)*BLOCK_SIZE
#define BLOCK_NUM (1<<7)
#define inode_table(i) BLOCK(0)+(i)*INODE_SIZE
#define data(i) BLOCK(16)+(i)*BLOCK_SIZE

struct dir_entry {
    int exists;
    int inode;
    int rec_len;
    int name_len;
    int file_type;
};
typedef struct dir_entry dir_entry_t;

int get_free_data(filesystem_t *fs, device_t *dev){
    int tmp = 1;
    for (int i=0;i<BLOCK_NUM;++i){
        dev->ops->read(dev, data(i), &tmp, sizeof(int));
        if (tmp==0) {
            tmp = 1;
            dev->ops->write(dev, data(i), &tmp, sizeof(int));
            return i;
        }
    }
    return -1;
}

void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    Log("size=%ld", sizeof(ext2_inode_t));
    ext2_inode_t *root = pmm->alloc(sizeof(ext2_inode_t));
    memset(root, 0, sizeof(root));
    root->exists = 1;
    root->type = DR;
    unsigned short per = R_OK|W_OK|X_OK;
    root->permission = per;
    root->len = 1;
    root->link[0] = get_free_data(fs, dev);

    dir_entry_t dot = pmm->alloc(sizeof(dir_entry_t));
    dot->exists = 1;
    dot->inode = 0;
    dot->rec_len = sizeof(dir_entry_t)+8*name_len;
    dot->name_len = 1;
    dot->file_type = DR;
    dev->ops->write(dev, data(root->link[0])+)

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
