#include <common.h>
#include <vfs.h>
#include <devices.h>

void *balloc(int size){
    void *ret = pmm->alloc(size);
    ret = memset(ret, 0, size);
    return ret;
}

// 1 if splited, 0 if done
int split(const char *path, char **pre, char **post){
    int ret = 0, len = strlen(path);
    for (int i=0;i<len;++i){
        if (path[i]=='/'){
            ret = 1;
            *pre = pmm->alloc(i+2);
            strncpy(*pre, path, i+1);
            (*pre)[i+1] = '\0';
            *post = pmm->alloc(len-i+2);
            strncpy(*post, path+i+1, len-i+1);
            break;
        }
    }
    return ret;
}
int split2(const char *path, char **pre, char **post){
    int ret = 0, len = strlen(path);
    for (int i=len-1;i>=0;--i){
        if (path[i]=='/'){
            ret = 1;
            *pre = pmm->alloc(i+2);
            strncpy(*pre, path, i+1);
            (*pre)[i+1] = '\0';
            *post = pmm->alloc(len-i+2);
            strncpy(*post, path+i+1, len-i+1);
            break;
        }
    }
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

#define LOGBLOCK() \
    LogBlock(IMAP, dev);\
    LogBlock(DMAP, dev);\
    LogBlock(ITABLE, dev);\
    LogBlock(DATA_B, dev);\
    LogBlock(DATA_B+1,dev)

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
#define INODE_BYTES (1<<7)
#define TABLE(i) (BLOCK(ITABLE)+(i)*INODE_BYTES)
enum TYPE {NF, DR, LK, MP};
struct ext2_inode {
  uint32_t index; //index of inode
  uint16_t type; //Type of this inode
  uint16_t permission; //Permission of this inode
  uint32_t size; //Size of file
  uint32_t len; //Number of link
  uint32_t link[28];
}__attribute__((packed));
typedef struct ext2_inode ext2_inode_t;

ext2_inode_t* ext2_create_inode(device_t *dev, uint8_t type, uint8_t per){
    int index_inode = free_map(dev, IMAP);
    write_map(dev, IMAP, index_inode, 1);
    ext2_inode_t *inode = (ext2_inode_t *)(pmm->alloc(sizeof(ext2_inode_t)));
    inode->index = index_inode;
    inode->type = type;
    inode->permission = per;
    inode->size = 0;
    inode->len = 0;
    return inode;
}

int ext2_dir_search(device_t *, ext2_inode_t*, const char*);
ext2_inode_t* ext2_lookup_dir(device_t *dev, const char *name){
    ext2_inode_t *inode = (ext2_inode_t *)(pmm->alloc(sizeof(ext2_inode_t)));
    dev->ops->read(dev, TABLE(0), inode, INODE_BYTES);
    char *pre = NULL, *post = NULL, *tmp;
    tmp = pmm->alloc(strlen(name)+1);
    strcpy(tmp, name);
    int splited = split(tmp, &pre, &post);
    while (splited){
        strcpy(tmp, post);
        pmm->free(pre); pmm->free(post);
        splited = split(tmp, &pre, &post);
        Log("pre=%s post=%s", pre, post);
        int inode_index = ext2_dir_search(dev, inode, pre);
        dev->ops->read(dev, TABLE(inode_index), inode, INODE_BYTES);
    }

    return inode;
}

/*======== DATA ===========*/
#define DATA_B ITABLE+ITABLE_NUM
#define DATA(i) BLOCK(DATA_B)+(i)*BLOCK_BYTES
void ext2_append_data(device_t *dev, ext2_inode_t* inode, const void *buf, int size){
    int add_size = size;
    int left = inode->len*BLOCK_BYTES - inode->size;
    assert(left>=0);
    if (left>0){
        int offset = inode->size - (inode->len - 1)*BLOCK_BYTES;
        int towrite = left<size?left:size;
        dev->ops->write(dev, DATA(inode->link[inode->len-1])+offset, buf, towrite);
        size-=towrite;
    }
    while (size){
        inode->link[inode->len] = free_map(dev, DMAP);
        write_map(dev, DMAP, inode->link[inode->len], 1);
        inode->len ++;
        int towrite = BLOCK_BYTES<size?BLOCK_BYTES:size;
        dev->ops->write(dev, DATA(inode->link[inode->len-1]), buf, towrite);
        size -= towrite;
    }
    inode->size += add_size;
}

/*======== DIR ============*/
struct dir_entry {
    uint32_t inode;
    uint32_t rec_len;
    uint32_t name_len;
    uint32_t file_type;
};
typedef struct dir_entry dir_entry_t;

void ext2_create_entry(device_t *dev, ext2_inode_t* inode, ext2_inode_t* entry_inode, const char* entry_name, uint32_t type){
    dir_entry_t* dir = balloc(sizeof(dir_entry_t));
    dir->inode = entry_inode->index;
    uint32_t name_len = strlen(entry_name);
    dir->name_len = name_len+1;
    dir->rec_len = sizeof(dir_entry_t)+dir->name_len;
    dir->file_type = type;

    ext2_append_data(dev, inode, dir, sizeof(dir_entry_t));
    ext2_append_data(dev, inode, entry_name, dir->name_len);
}

#define OFFSET_BLOCK(offset) (inode->link[(offset)/BLOCK_BYTES])
#define OFFSET_REMAIN(offset) ((offset)%BLOCK_BYTES)
int ext2_dir_search(device_t *dev, ext2_inode_t* inode, const char* name){
    dir_entry_t* cur = pmm->alloc(sizeof(dir_entry_t));
    int offset = 0;
    while (offset < inode->size){
        dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
        char *tmp_name = pmm->alloc(cur->name_len+1);
        dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset)+cur->name_len, cur, cur->name_len);
        pmm->free(tmp_name);

        if (strncmp(name, tmp_name, cur->name_len)==0){
            break;
        }
        offset += cur->rec_len;
    }
    pmm->free(cur);
    return cur->inode;
}

void ext2_create_dir(device_t *dev, const char *name, int isroot){
    unsigned short per = R_OK|W_OK|X_OK;
    ext2_inode_t* dir;
    if (isroot){
        dir = ext2_create_inode(dev, DR, per);
        ext2_create_entry(dev, dir, dir, ".", DR);
        ext2_create_entry(dev, dir, dir, "..", DR);
    } else {
        char *pre, *post;
        split2(name, &pre, &post);
        Log("pre=%s post=%s", pre, post);

        dir = ext2_create_inode(dev, DR, per);
        ext2_create_entry(dev, dir, dir, ".", DR);
        ext2_create_entry(dev, dir, dir, "..", DR);

        ext2_inode_t* father = ext2_lookup_dir(dev, pre);
        ext2_create_entry(dev, father, dir, post, DR);
        dev->ops->write(dev, TABLE(father->index), father, INODE_BYTES);
        pmm->free(father);
    }
    dev->ops->write(dev, TABLE(dir->index), dir, INODE_BYTES);
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

    bzero(DATA_B);
    ext2_create_dir(dev, name, 1);
    ext2_create_dir(dev, "/bin", 0);
    ext2_create_dir(dev, "/test", 0);
    //ext2_create_dir(dev, "/etc");

    LOGBLOCK();
    assert(0);
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
