#include <common.h>
#include <vfs.h>
#include <devices.h>


/*========== BLOCK =============*/
static void bzero(device_t* dev, int x){
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
#define MAP(block,i) (BLOCK(block)+(i)/8)
static int read_map(device_t *dev, int block, int i){
    uint8_t m = 1<<(i%8), b;
    dev->ops->read(dev, MAP(block,i), &b, sizeof(uint8_t));
    return ((b&m)!=0);
}

static int write_map(device_t* dev, int block, int i, uint8_t x){
    assert(x==0||x==1);
    uint8_t m = 1<<(i%8), b;
    dev->ops->read(dev, MAP(block, i), &b, sizeof(uint8_t));
    assert(read_map(dev, block, i)!=x);
    if (x==1) b |= m; else b &= ~m;
    dev->ops->write(dev, MAP(block, i), &b, sizeof(uint8_t));
    assert(read_map(dev, block, i)==x);
    return 0;
}

static int free_map(device_t* dev, int block){
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
struct ext2_inode {
  uint32_t index; //index of inode
  uint16_t type; //Type of this inode
  uint16_t permission; //Permission of this inode
  uint32_t size; //Size of file
  uint32_t len; //Number of link
  uint32_t id; //File id
  uint32_t dir_len; //Special for dir
  uint32_t link_num;
  uint32_t link[25];
}__attribute__((packed));
uint32_t gid;

void ext2_inode_remove(device_t *dev, ext2_inode_t* inode){
    write_map(dev, IMAP, inode->index, 0);
    for (int i=0;i<inode->len;++i){
        write_map(dev, DMAP, inode->link[i], 0);
    }
}

ext2_inode_t* ext2_inode_create(device_t *dev, uint8_t type, uint8_t per){
    int index_inode = free_map(dev, IMAP);
    write_map(dev, IMAP, index_inode, 1);
    ext2_inode_t *inode = (ext2_inode_t *)(balloc(sizeof(ext2_inode_t)));
    inode->index = index_inode;
    inode->type = type;
    inode->permission = per;
    inode->size = 0;
    inode->len = 0;
    inode->id = ++gid;
    inode->dir_len = 0;
    inode->link_num = 1;
    return inode;
}

const  char *mp = "/";
ext2_inode_t* ext2_inode_lookup(device_t *dev, const char *name){
    char* tmp = pmm->alloc(strlen(name)+1);
    strcpy(tmp, name);
    strip(tmp);
    
    ext2_inode_t *inode = (ext2_inode_t *)(pmm->alloc(sizeof(ext2_inode_t)));
    dev->ops->read(dev, TABLE(0), inode, INODE_BYTES);
    
    char *pre, *post;
    pre = rootdir(tmp);
    post = postname(tmp);

    while (pre!=NULL){
        int inode_index = ext2_dir_lookup(dev, inode, pre);
        if (inode_index>=0){
            dev->ops->read(dev, TABLE(inode_index), inode, INODE_BYTES);
        } else {
            inode = NULL;
            break;
        }

        if (tmp) pmm->free(tmp); 
        if (pre) pmm->free(pre);
        tmp = post; 
        pre = rootdir(tmp);
        post = postname(tmp);
    }
    return inode;
}

/*======== DATA ===========*/
ssize_t ext2_data_write(device_t *dev, ext2_inode_t* inode, const void *buf, int size, int offset){
    int bytes = size;
    int buf_offset = 0; //offset pointer of buffer
    int original_len = inode->len;
    int towrite; //bytes to write once
    int first = 1;

    while (bytes>0){
        if (offset < original_len*BLOCK_BYTES){
            towrite = BLOCK_BYTES;
            if (first) {
                first = 0;
                towrite -= OFFSET_REMAIN(offset);
            }
            towrite = min(towrite, bytes);
        } else {
            inode->link[inode->len] = free_map(dev, DMAP);
            write_map(dev, DMAP, inode->link[inode->len], 1);
            inode->len ++;
            towrite = BLOCK_BYTES<bytes?BLOCK_BYTES:bytes;
        }

        dev->ops->write(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), buf+buf_offset, towrite);
        bytes -= towrite;
        buf_offset += towrite;
    }

    int update_size = offset + size;
    if (update_size>inode->size){
        inode->size = update_size;
        dev->ops->write(dev, TABLE(inode->index), inode, INODE_BYTES);
    }
    return size;
}

ssize_t ext2_data_read(device_t *dev, ext2_inode_t* inode, void *buf, int size, int offset){
    int cnt = offset/BLOCK_BYTES, buf_offset = 0;
    int left;
    int first = 1;
    while (offset < inode->size && size){
        if (cnt<inode->len-1) {
            if (first) {
                first = 0;
                left = BLOCK_BYTES - OFFSET_REMAIN(offset);
            } else left = BLOCK_BYTES;
        } else {
            if (first){
                first = 0;
                left =  inode->size - (inode->len-1)*BLOCK_BYTES - OFFSET_REMAIN(offset);
            } else left = inode->size - (inode->len-1);
        }    
        left = min(left, size);
        dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), buf+buf_offset, left);
        size-=left;
        offset+=left;
        buf_offset += left;
        cnt++;
    }
    return buf_offset;
}

/*======== DIR ============*/
struct dir_entry {
    uint32_t inode;
    uint32_t rec_len;
    uint32_t name_len;
    uint32_t file_type;
};
typedef struct dir_entry dir_entry_t;

static void ext2_create_entry(device_t *dev, ext2_inode_t* inode, ext2_inode_t* entry_inode, const char* entry_name, uint32_t type){
    dir_entry_t* dir = balloc(sizeof(dir_entry_t));
    dir->inode = entry_inode->index;
    uint32_t name_len = strlen(entry_name);
    dir->name_len = name_len+1;
    dir->rec_len = sizeof(dir_entry_t)+dir->name_len;
    dir->file_type = type;

    inode->dir_len++;

    ext2_append_data(dev, inode, dir, sizeof(dir_entry_t));
    ext2_append_data(dev, inode, entry_name, dir->name_len);
    pmm->free(dir);
}

int ext2_dir_lookup(device_t *dev, ext2_inode_t* inode, const char* name){
    dir_entry_t* cur = pmm->alloc(sizeof(dir_entry_t));
    int finded = 0, offset = 0;
    while (offset < inode->size){
        ext2_data_read(dev, inode, cur, sizeof(dir_entry_t), offset);
        if (cur->file_type !=XX){
            char *tmp_name = pmm->alloc(cur->name_len+1);
            Log("tmp:)%d",tmp_name);

            ext2_data_read(dev, inode, tmp_name, cur->name_len, offset+sizeof(dir_entry_t));
            
            if (strcmp(name, tmp_name)==0){
                finded =1;
                break;
            }
Log("tmp:)%d,%s",tmp_name,tmp_name);
            pmm->free(tmp_name);
            Log("tmp:)%d",tmp_name);
        }
        offset += cur->rec_len;
    }
    Log("lll");
    pmm->free(cur);
    if (finded)
        return cur->inode;
    else
        return -1;
}

void ext2_dir_remove(device_t *dev, ext2_inode_t* inode, int index){
    dir_entry_t* cur = pmm->alloc(sizeof(dir_entry_t));
    int offset = 0;
    while (offset < inode->size){
        dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
        if (cur->inode == index && cur->file_type!=XX){
            cur->file_type = XX;
            break;
        }
        offset += cur->rec_len;
    }
    dev->ops->write(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
    pmm->free(cur);

    inode->dir_len --;
    dev->ops->write(dev, TABLE(inode->index), inode, INODE_BYTES);
}

int ext2_create_file(device_t *dev, const char *name, int isroot, int per, int type){
    ext2_inode_t* dir;
    if (isroot){
        dir = ext2_inode_create(dev, type, per);
        ext2_create_entry(dev, dir, dir, ".", DR);
        ext2_create_entry(dev, dir, dir, "..", DR);
    } else {
        char *pre, *post;
        char *tmp = balloc(strlen(name)+1);
        strcpy(tmp, name+strlen(mp));
        pre = alldir(tmp);
        post = filename(tmp);

        ext2_inode_t* father = ext2_inode_lookup(dev, pre);
        if (father==NULL) return -1;

        dir = ext2_inode_create(dev, type, per);
        if (type == DR){
            ext2_create_entry(dev, dir, dir, ".", DR);
            ext2_create_entry(dev, dir, father, "..", DR);
        }

        ext2_create_entry(dev, father, dir, post, type);
        dev->ops->write(dev, TABLE(father->index), father, INODE_BYTES);
        pmm->free(father);
    }
    dev->ops->write(dev, TABLE(dir->index), dir, INODE_BYTES);
    pmm->free(dir);
    return 0;
}

/*======== API ============*/
inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags);
void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    printf("==== EXT2 INFO ====\n Block Size:%#lx\n Inode Nums:%d\nInode Start:%d\n Inode Size:%#lx\n Data Start:%d\n",BLOCK_BYTES, ITABLE_NUM, ITABLE, sizeof(ext2_inode_t), DATA_B);
    //clear
    bzero(dev, IMAP);
    bzero(dev, DMAP);
    for (int i=ITABLE; i<ITABLE+ITABLE_NUM; ++i){
        bzero(dev, i);
    }

    ext2_create_dir(dev, name, 1);
    ext2_create_dir(dev, "/bin", 0);
    ext2_create_dir(dev, "/home", 0);
    ext2_create_dir(dev, "/usr", 0);
    ext2_create_dir(dev, "/usr/bin", 0);
    ext2_create_dir(dev, "/etc", 0);
    ext2_create_file(dev, "/etc/passwd", 0, R_OK|W_OK|X_OK, NF);
 
    const char *words = "zhengzangw:x:1000:1000:zhengzangw,,,:/home/zhengzangw:/bin/awsh";
    inode_t* tmp = ext2_lookup(fs, "etc/passwd", 0);
    assert(tmp!=NULL);
    ext2_append_data(dev, tmp->fs_inode, words, strlen(words));

    //LOGBLOCK();
    //assert(0);
}

inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags){
    ext2_inode_t* tmp = ext2_inode_lookup(fs->dev, name);
    if (tmp){
        inode_t* ret = balloc(sizeof(inode_t));
        ret->fs = fs;
        ret->fs_inode = tmp;
        ret->ops = &ext2_inodeops;

        ret->permission = ((ext2_inode_t*)ret->fs_inode)->permission;
        ret->size = ((ext2_inode_t*)ret->fs_inode)->size;
        ret->id = ((ext2_inode_t*)ret->fs_inode)->id;
        ret->type = ((ext2_inode_t*)ret->fs_inode)->type;
        ret->dir_len = ((ext2_inode_t*)ret->fs_inode)->dir_len;
        ret->link_num = ((ext2_inode_t*)ret->fs_inode)->link_num;

        return ret;
    } else {
        return NULL;
    }
}

int ext2_close(inode_t *inode){
    return 0;
}

int ext2_mkdir(filesystem_t *fs, const char *name){
    return ext2_create_dir(fs->dev, name, 0);
}

int ext2_rmdir(filesystem_t *fs, const char *name){
    if (strlen(name)==1) return -1;

    char *pre = NULL, *post = NULL;
    char tmp[128];
    strcpy(tmp, name);
    split2(tmp, &pre, &post);

    //Get parent dir inode
    ext2_inode_t* dir = ext2_inode_lookup(fs->dev, pre);
    int index;
    if (!dir){
        pmm->free(pre); pmm->free(post);
        return -1;
    }

    index = ext2_dir_lookup(fs->dev, dir, post);
    pmm->free(pre); pmm->free(post);
    if (index<0){
        return -1;
    }

    ext2_inode_t *inode = pmm->alloc(sizeof(ext2_inode_t));
    fs->dev->ops->read(fs->dev, TABLE(index), inode, INODE_BYTES);
    if (inode->type==DR && inode->dir_len>2){
        return -1;
    }
    ext2_dir_remove(fs->dev, dir, index);
    pmm->free(dir);
    if (inode->link_num==1){
        ext2_inode_remove(fs->dev, inode);
    } else {
        inode->link_num -= 1;
        fs->dev->ops->write(fs->dev, TABLE(index), inode, INODE_BYTES);
    }
    pmm->free(inode);

    return 0;
}

int ext2_create(filesystem_t *fs, const char *name){
    return ext2_create_file(fs->dev, name, 0, R_OK|W_OK|X_OK, NF);
}

fsops_t ext2_ops = {
    .init = ext2_init,
    .lookup = ext2_lookup,
    .close = ext2_close,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .unlink = ext2_rmdir,
    .create = ext2_create,
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
    int offset = 0;
    int cnt = 0 ,ret = 0, buf_offset = 0;
    switch (file->inode->type){
        case DR:
            while (cnt < inode->dir_len && size){
                dir_entry_t* cur = pmm->alloc(sizeof(dir_entry_t));
                dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), cur, sizeof(dir_entry_t));
                if (cur->file_type!=XX){
                    char *tmp_name = pmm->alloc(cur->name_len+1);
                    int name_offset = offset+sizeof(dir_entry_t);
                    dev->ops->read(dev, DATA(OFFSET_BLOCK(name_offset))+OFFSET_REMAIN(name_offset), tmp_name, cur->name_len);

                    cnt ++;
                    if (cnt>file->offset){
                        size--;
                        ret++;
                        strncpy(buf+buf_offset, tmp_name, strlen(tmp_name));
                        strcat(buf, "\n");
                        buf_offset += strlen(tmp_name)+1;
                    }
                    pmm->free(tmp_name);
                }

                offset += cur->rec_len;
                pmm->free(cur);
            }
            buf[buf_offset] = '\0';
            return ret;

        case NF:
            offset = file->offset;
            cnt = offset/BLOCK_BYTES;
            int left;
            int first = 1;
            while (offset < inode->size && size){
                if (cnt<inode->len-1) {
                    if (first) {
                        first = 0;
                        left = BLOCK_BYTES - OFFSET_REMAIN(offset);
                    } else left = BLOCK_BYTES;
                } else {
                    if (first){
                        first = 0;
                        left =  inode->size - (inode->len-1)*BLOCK_BYTES - OFFSET_REMAIN(offset);
                    } else left = inode->size - (inode->len-1);
                }
                dev->ops->read(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), buf+buf_offset, left);
                size-=left;
                offset+=left;
                buf_offset += left;
                cnt++;
            }
            if (buf[buf_offset-1] !='\n') {
                buf[buf_offset] = '\n';
                buf[buf_offset+1] = '\0';
            } else {
                buf[buf_offset] = '\0';
            }
    }
    return 0;
}

#define BLOCK_COVER(offset) (offset/BLOCK_BYTES)
ssize_t ext2_inode_write(file_t *file, const char *buf, size_t size){
    ext2_inode_t* inode = file->inode->fs_inode;
    device_t* dev = file->inode->fs->dev;

    int bytes = size;
    int offset = file->offset; //offset pointer of file
    int buf_offset = 0; //offset pointer of buffer
    int original_len = inode->len;
    int towrite; //bytes to write once
    int first = 1;

    switch (inode->type){
        case DR: assert(0);
        case NF:
            while (bytes>0){
                Logint(bytes);
                Logint(offset);
                if (offset < original_len*BLOCK_BYTES){
                    towrite = BLOCK_BYTES;
                    if (first) {
                       first = 0;
                       towrite -= OFFSET_REMAIN(offset);
                    }
                    towrite = min(towrite, bytes);
                } else {
                    inode->link[inode->len] = free_map(dev, DMAP);
                    write_map(dev, DMAP, inode->link[inode->len], 1);
                    inode->len ++;
                    towrite = BLOCK_BYTES<bytes?BLOCK_BYTES:bytes;
                }

                dev->ops->write(dev, DATA(OFFSET_BLOCK(offset))+OFFSET_REMAIN(offset), buf+buf_offset, towrite);
                bytes -= towrite;
                buf_offset += towrite;
        }

        int update_size = offset + size;
        if (update_size>inode->size){
            inode->size = update_size;
            dev->ops->write(dev, TABLE(inode->index), inode, INODE_BYTES);
        }
    }

    return size;
}

int ext2_inode_link(file_t *file, const char *name){
    ext2_inode_t* inode = file->inode->fs_inode;
    device_t* dev = file->inode->fs->dev;
    char *pre = NULL, *post = NULL;
    split2(name, &pre, &post);
    ext2_inode_t* father = ext2_inode_lookup(dev, pre);
    ext2_create_entry(dev, father, inode, post, NF);
    inode->link_num++;
    dev->ops->write(dev, TABLE(inode->index), inode, INODE_BYTES);
    return 0;
}

off_t ext2_inode_lseek(file_t *file, off_t offset, int whence){
    off_t t_offset = -1;
    int size = file->inode->size;
    switch (whence){
        case S_SET: t_offset = max(min(offset, size), 0); break;
        case S_CUR: t_offset = max(min(file->offset+offset, size), 0); break;
        case S_END: t_offset = max(min(size+offset, size), 0); break;
    }
    file->offset = t_offset;
    Logint(t_offset);
    return t_offset;
}


inodeops_t ext2_inodeops = {
    .open = ext2_inode_open,
    .close = ext2_inode_close,
    .read = ext2_inode_read,
    .write = ext2_inode_write,
    .link = ext2_inode_link,
    .lseek = ext2_inode_lseek,
};
