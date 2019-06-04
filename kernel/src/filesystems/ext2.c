#include <common.h>
#include <vfs.h>
#include <devices.h>

/*========== BLOCK ===============*/
#define BLOCK_BYTES (1<<9)
#define BLOCK(x) (x)*BLOCK_BYTES
void bzero(int x, device_t* dev){
    void *zeros = pmm->alloc(BLOCK_BYTES);
    for (int i=0;i<BLOCK_BYTES;++i){
        *((char *)zeros+i) = 0;
    }
    dev->ops->write(dev, BLOCK(x), &zeros, BLOCK_BYTES);
    pmm->free(zeros);
}

void LogBlock(int x, device_t* dev) {
    void *logs = pmm->alloc(BLOCK_BYTES);
    dev->ops->read(dev, BLOCK(x), logs, BLOCK_BYTES);
    printf("======== LOG BLOCK =======\n");
    for (int i=0;i<BLOCK_BYTES;++i){
        printf("%02x ", *((char *)logs+i));
        if ((i+1)%(1<<6)==0) printf("\n");
    }
    printf("======== LOG ENDED =======\n");
    pmm->free(logs);
}

void *balloc(int size){
    void *ret = pmm->alloc(size);
    ret = memset(ret, 0, size);
    return ret;
}

/*========== IMAP,DMAP ===========*/
#define IMAP 0
#define DMAP 1
#define MAP(block,i) (BLOCK(block)+(i)/8)
int read_map(int block, int i, device_t* dev){
    uint8_t m = 1<<(i%8);
    uint8_t b;
    dev->ops->read(dev, MAP(block,i), &b, sizeof(uint8_t));
    return (b&m)==1;
}
int write_map(int block, int i, uint8_t x, device_t* dev){
    assert(x==0||x==1);
    uint8_t m = 1<<(i%8);
    uint8_t b;
    dev->ops->read(dev, MAP(block, i), &b, sizeof(uint8_t));
    assert(read_map(block, i, dev)!=x);
    if (x==1) b |= m; else b &= ~m;
    dev->ops->write(dev, MAP(block, i), &b, sizeof(uint8_t));
    return 0;
}
int free_map(int block, device_t* dev){
    int ret = -1;
    for (int i=0;i<BLOCK_BYTES;++i){
        if (read_map(block, i, dev)==1) {
            ret = i;
            break;
        }
    }
    return ret;
}

/*======== ITABLE =========*/
#define ITABLE 3
#define ITABLE_NUM 2
#define INODE_BYTES (1<<8)
#define TABLE(i) BLOCK(ITABLE)+(i)*INODE_BYTES
enum TYPE {NF, DR, LK, MP};
struct ext2_inode {
  uint32_t exists; //Whether this inode exists
  unsigned short type; //Type of this inode
  unsigned short permission; //Permission of this inode
  uint32_t size; //Size of file
  uint32_t len; //Number of link
  uint32_t link[60];
}__attribute__((packed));
typedef struct ext2_inode ext2_inode_t;

/*======== DATA ===========*/
#define DATA_B ITABLE+ITABLE_NUM
#define DATA(i) BLOCK(DATA_B)+(i)*BLOCK_BYTES

/*======== DIR ============*/
struct dir_entry {
    int inode;
    int rec_len;
    int name_len;
    int file_type;
};
typedef struct dir_entry dir_entry_t;

/*======== API ============*/
void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    Log("EXT2 INFO: inode size=%ld", sizeof(ext2_inode_t));

    //clear
    bzero(IMAP, dev);
    bzero(DMAP, dev);
    for (int i=ITABLE; i<ITABLE+ITABLE_NUM; ++i){
        bzero(i, dev);
    }

    ext2_inode_t *root = (ext2_inode_t *)(pmm->alloc(sizeof(ext2_inode_t)));
    root->exists = 1;
    root->type = DR;
    unsigned short per = R_OK|W_OK|X_OK;

    root->permission = per;
    root->len = 1;
    root->link[0] = free_map(DMAP, dev);
    bzero(DATA(root->link[0]), dev);

    /*
    dir_entry_t dir = balloc(sizeof(dir_entry_t));
    dir->inode = 0;
    name_len = strlen(name);
    dir->name_len = name_len+1;
    dir->rec_len = sizeof(dir_entry_t)+dir->name_len;
    dir->file_type = DR;
    dev->ops->write(dev, data(root->link[0]))
    */

    dev->ops->write(dev, TABLE(0), &root, INODE_BYTES);
    pmm->free(root);

    LogBlock(IMAP, dev);
    LogBlock(DMAP, dev);
    LogBlock(ITABLE, dev);
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
