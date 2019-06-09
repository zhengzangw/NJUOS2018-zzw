#include <common.h>
#include <vfs.h>
#include <devices.h>

#define LOG_NUM 4
#define LOGBLOCK() \
    for (int i=0;i<DATA_B+LOG_NUM;++i)\
        LogBlock(dev, i)


/*========== BLOCK ===============*/
#define BLOCK_BYTES (1<<8)
#define BLOCK(x) ((x)*BLOCK_BYTES)
void bzero(device_t* dev, int x){
    void *zeros = balloc(BLOCK_BYTES);
    dev->ops->write(dev, BLOCK(x), zeros, BLOCK_BYTES);
    pmm->free(zeros);
}

void LogBlock(device_t* dev, int x) {
    void *logs = pmm->alloc(BLOCK_BYTES);
    dev->ops->read(dev, BLOCK(x), logs, BLOCK_BYTES);
    printf("======== LOG BLOCK %d ========\n", x);
    for (int i=0;i<BLOCK_BYTES;++i){
        printf("%02x ", *((unsigned char *)logs+i));
        if ((i+1)%(1<<4)==0) printf("\n");
    }
    printf("######## LOG ENDED %d ########\n", x);
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
#define ITABLE 2
#define ITABLE_NUM 3
#define INODE_BYTES (1<<7)
#define TABLE(i) (BLOCK(ITABLE)+(i)*INODE_BYTES)
enum TYPE {NF, DR, LK, MP};
struct ext2_inode {
  uint32_t index; //index of inode
  uint16_t type; //Type of this inode
  uint16_t permission; //Permission of this inode
  uint32_t size; //Size of file
  uint32_t len; //Number of link
  uint32_t id;
  uint32_t link[27];
}__attribute__((packed));
uint32_t gid;
typedef struct ext2_inode ext2_inode_t;

ext2_inode_t* ext2_create_inode(device_t *dev, uint8_t type, uint8_t per){
    int index_inode = free_map(dev, IMAP);
    write_map(dev, IMAP, index_inode, 1);
    ext2_inode_t *inode = (ext2_inode_t *)(balloc(sizeof(ext2_inode_t)));
    inode->index = index_inode;
    inode->type = type;
    inode->permission = per;
    inode->size = 0;
    inode->len = 0;
    inode->id = ++gid;
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
    //Log("tmp=%s name=%s splited=%d", tmp, name, splited);
    while (splited){
        strcpy(tmp, post);
        pmm->free(pre); pmm->free(post);
        splited = split(tmp, &pre, &post);
        //Log("pre=%s post=%s splited=%d", pre, post, splited);
        int inode_index = ext2_dir_search(dev, inode, pre);
        dev->ops->read(dev, TABLE(inode_index), inode, INODE_BYTES);
    }

    return inode;
}

ext2_inode_t* ext2_lookup_inode(device_t *dev, const char *name){
    char *pre = NULL, *post = NULL, *tmp;
    tmp = pmm->alloc(strlen(name)+1);
    strcpy(tmp, name);
    int splited = split2(tmp, &pre, &post);
    Log("tmp=%s pre=%s post=%s splited=%d", tmp, pre, post, splited);
    ext2_inode_t* dir = ext2_lookup_dir(dev, pre);
    int index = ext2_dir_search(dev, dir, post);
    pmm->free(dir);
    ext2_inode_t* ret = pmm->alloc(sizeof(ext2_inode_t));
    dev->ops->read(dev, TABLE(index), ret, INODE_BYTES);
    return ret;
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
    int finded = 0;
    int offset = 0;
    while (offset < inode->size){
        dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
        char *tmp_name = pmm->alloc(cur->name_len+1);
        int name_offset = offset+sizeof(dir_entry_t);
        dev->ops->read(dev, DATA(OFFSET_BLOCK(name_offset))+OFFSET_REMAIN(name_offset), tmp_name, cur->name_len);
        //Log("tmp_name=%s name=%s", tmp_name, name);

        if (strncmp(name, tmp_name, cur->name_len-1)==0){
            finded =1;
            break;
        }
        pmm->free(tmp_name);
        offset += cur->rec_len;
    }
    pmm->free(cur);
    assert(finded==1);
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
inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags);
void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    Log("EXT2 INFO: inode size=%ld", sizeof(ext2_inode_t));
    //clear
    bzero(dev, IMAP);
    bzero(dev, DMAP);
    for (int i=ITABLE; i<ITABLE+ITABLE_NUM; ++i){
        bzero(dev, i);
    }

    ext2_create_dir(dev, name, 1);
    ext2_create_dir(dev, "/bin", 0);
    ext2_create_dir(dev, "/test", 0);
    ext2_create_dir(dev, "/bin/a.txt", 0);

    //LOGBLOCK();
}

inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags){
    inode_t* ret = balloc(sizeof(inode_t));
    ret->fs = fs;
    ret->fs_inode = ext2_lookup_inode(fs->dev, name);
    ret->ops = &ext2_inodeops;

    ret->permission = ((ext2_inode_t*)ret->fs_inode)->permission;
    ret->size = ((ext2_inode_t*)ret->fs_inode)->size;
    ret->id = ((ext2_inode_t*)ret->fs_inode)->id;
    ret->type = ((ext2_inode_t*)ret->fs_inode)->type;

    return ret;
}

int ext2_close(inode_t *inode){
    return 0;
}

fsops_t ext2_ops = {
    .init = ext2_init,
    .lookup = ext2_lookup,
    .close = ext2_close,
};

/* ===== Inode API ====== */

int ext2_inode_open(file_t *file, int flags){
    //ext2_inode_t inode = (ext2_inode_t*)file->inode->fs_inode;
    file->offset = 0;
    return 0;
}

int ext2_inode_close(file_t *file){
    return 0;
}

ssize_t ext2_inode_read(file_t *file, char *buf, size_t size){
    ext2_inode_t* inode = file->inode->fs_inode;
    device_t* dev = file->inode->fs->dev;
    int offset = file->offset;
            int cnt = 0 ,ret = 0, buf_offset = 0;
    switch (file->inode->type){
        case DR:
            while (offset < inode->size && size){
                dir_entry_t* cur = pmm->alloc(sizeof(dir_entry_t));
                dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
                char *tmp_name = pmm->alloc(cur->name_len+1);
                int name_offset = offset+sizeof(dir_entry_t);
                dev->ops->read(dev, DATA(OFFSET_BLOCK(name_offset))+OFFSET_REMAIN(name_offset), tmp_name, cur->name_len);
                Log("Name = %s", tmp_name);

                cnt ++;
                if (cnt>file->offset){
                    size--;
                    ret++;
                    strncpy(buf+buf_offset, tmp_name, strlen(tmp_name));
                    strcat(buf, " ");
                    buf_offset += strlen(tmp_name)+1;
                }

                pmm->free(tmp_name);
                pmm->free(cur);
                offset += cur->rec_len;
            }
            return ret;

        case NF:
        default: assert(0);
    }
    return 0;
}

inodeops_t ext2_inodeops = {
    .open = ext2_inode_open,
    .close = ext2_inode_close,
    .read = ext2_inode_read,
    .write = NULL,
    .lseek = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .link = NULL,
    .unlink = NULL,
};
