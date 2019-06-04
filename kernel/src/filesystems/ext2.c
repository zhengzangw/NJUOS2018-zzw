#include <common.h>
#include <vfs.h>
#include <devices.h>

void *balloc(int size){
    void *ret = pmm->alloc(size);
    ret = memset(ret, 0, size);
    return ret;
}

/*========== BLOCK ===============*/
#define BLOCK_BYTES (1<<9)
#define BLOCK(x) ((x)*BLOCK_BYTES)
#define bzero(x) bzero_dev(dev, x)
void bzero_dev(device_t* dev, int x){
    void *zeros = balloc(BLOCK_BYTES);
    dev->ops->write(dev, BLOCK(x), zeros, BLOCK_BYTES);
    pmm->free(zeros);
}

void LogBlock(int x, device_t* dev) {
    void *logs = pmm->alloc(BLOCK_BYTES);
    dev->ops->read(dev, BLOCK(x), logs, BLOCK_BYTES);
    printf("======== LOG BLOCK %d ========\n", x);
    for (int i=0;i<BLOCK_BYTES;++i){
        printf("%02x ", *((unsigned char *)logs+i));
        if ((i+1)%(1<<4)==0) printf("\n");
    }
    printf("======== LOG ENDED %d ========\n", x);
    pmm->free(logs);
}

/*========== IMAP,DMAP ===========*/
#define IMAP 0
#define DMAP 1
#define MAP(block,i) (BLOCK(block)+(i)/8)
int read_map(device_t *dev, int block, uint8_t i){
    uint8_t m = 1<<(i%8), b;
    dev->ops->read(dev, MAP(block,i), &b, sizeof(uint8_t));
    return ((b&m)!=0);
}

int write_map(device_t* dev, int block, int i, uint8_t x){
    assert(x==0||x==1);
    uint8_t m = 1<<(i%8), b;
    dev->ops->read(dev, MAP(block, i), &b, sizeof(uint8_t));
    assert(read_map(dev, block, i)!=x);
    if (x==1) b |= m; else b &= ~m;
    dev->ops->write(dev, MAP(block, i), &b, sizeof(uint8_t));
    assert(read_map(dev, block, i)==x);
    return 0;
}

int free_map(device_t* dev, int block){
    int ret = -1;
    for (int i=0;i<BLOCK_BYTES;++i){
        if (read_map(dev, block, i)==0) {
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
#define TABLE(i) (BLOCK(ITABLE)+(i)*INODE_BYTES)
enum TYPE {NF, DR, LK, MP};
struct ext2_inode {
  uint32_t exists; //Whether this inode exists
  uint16_t type; //Type of this inode
  uint16_t permission; //Permission of this inode
  uint32_t size; //Size of file
  uint32_t len; //Number of link
  uint32_t link[60];
}__attribute__((packed));
typedef struct ext2_inode ext2_inode_t;

ext2_inode_t* ext2_create_inode(device_t *dev, uint8_t type, uint8_t per){
    int index_inode = free_map(dev, IMAP);
    write_map(dev, IMAP, 1);
    Logint(index_inode);
    ext2_inode_t *inode = (ext2_inode_t *)(pmm->alloc(sizeof(ext2_inode_t)));
    inode->exists = 1;
    inode->type = type;
    inode->permission = per;
    inode->len = 0;
    dev->ops->write(dev, TABLE(index_inode), inode, INODE_BYTES);
    return inode;
}

/*======== DATA ===========*/
#define DATA_B ITABLE+ITABLE_NUM
#define DATA(i) BLOCK(DATA_B)+(i)*BLOCK_BYTES

/*======== DIR ============*/
struct dir_entry {
    uint32_t inode;
    uint32_t rec_len;
    uint32_t name_len;
    uint32_t file_type;
};
typedef struct dir_entry dir_entry_t;

void ext2_create_dir(device_t *dev, const char *name){
    unsigned short per = R_OK|W_OK|X_OK;
    ext2_inode_t* dir = ext2_create_inode(dev, DR, per);
    pmm->free(dir);
}

/*======== API ============*/
void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    Log("EXT2 INFO: inode size=%ld", sizeof(ext2_inode_t));

    //clear
    bzero(IMAP);
    bzero(DMAP);
    for (int i=ITABLE; i<ITABLE+ITABLE_NUM; ++i){
        bzero(i);
    }
    ext2_create_dir(dev, name);

    /*
    dir_entry_t dir = balloc(sizeof(dir_entry_t));
    dir->inode = 0;
    name_len = strlen(name);
    dir->name_len = name_len+1;
    dir->rec_len = sizeof(dir_entry_t)+dir->name_len;
    dir->file_type = DR;
    dev->ops->write(dev, data(root->link[0]))
    */

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
