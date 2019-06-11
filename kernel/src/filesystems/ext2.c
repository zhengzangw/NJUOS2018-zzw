#include <common.h>
#include <vfs.h>
#include <devices.h>

/*========== BLOCK =============*/
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
#define MAP(block,i) (BLOCK(block)+(i)/8)
int read_map(device_t *dev, int block, int i){
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
void ext2_create_entry(device_t *dev, ext2_inode_t* inode, ext2_inode_t* entry_inode, const char* entry_name, uint32_t type){
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
            ext2_data_read(dev, inode, tmp_name, cur->name_len, offset+sizeof(dir_entry_t));
            
            if (strcmp(name, tmp_name)==0){
                finded =1;
                break;
            }
            pmm->free(tmp_name);
        }
        offset += cur->rec_len;
    }
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
        strcpy(tmp, name);
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
