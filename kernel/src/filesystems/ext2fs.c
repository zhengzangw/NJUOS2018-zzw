#include <common.h>
#include <vfs.h>
#include <devices.h>

/*======== API ============*/
void ext2_init(filesystem_t *fs, const char *name, device_t *dev){
    //printf("==== EXT2 INFO ====\n Block Size:%#lx\n Inode Nums:%d\nInode Start:%d\n Inode Size:%#lx\n Data Start:%d\n",BLOCK_BYTES, ITABLE_NUM, ITABLE, sizeof(ext2_inode_t), DATA_B);
    //clear
    bzero(dev, IMAP);
    bzero(dev, DMAP);
    for (int i=ITABLE; i<ITABLE+ITABLE_NUM; ++i){
        bzero(dev, i);
    }
    ext2_create_dir(dev, name, 1);
}

#define update_inode_attr(attr) inode->attr=EXT(inode)->attr
void update_inode(inode_t* inode, ext2_inode_t *extinode){
    if (inode->fs_inode) pmm->free(inode->fs_inode);
    inode->fs_inode = extinode;
    update_inode_attr(permission);
    update_inode_attr(size);
    update_inode_attr(id);
    update_inode_attr(type);
    update_inode_attr(dir_len);
    update_inode_attr(link_num);
}

inode_t* ext2_lookup(filesystem_t *fs, const char *name, int flags){
    ext2_inode_t* tmp = ext2_inode_lookup(fs->dev, name);
    if (tmp){
        inode_t* ret = balloc(sizeof(inode_t));
        ret->fs = fs;
        ret->ops = &ext2_inodeops;
        update_inode(ret, tmp);
        return ret;
    } else {
        return NULL;
    }
}

int ext2_close(inode_t *inode){
    pmm->free(EXT(inode));
    return 0;
}

int ext2_mkdir(filesystem_t *fs, const char *name){
    return ext2_create_dir(fs->dev, name, 0);
}

int ext2_create(filesystem_t *fs, const char *name){
    return ext2_create_file(fs->dev, name, 0, R_OK|W_OK|X_OK, NF);
}

int ext2_rm(filesystem_t *fs, const char *name){
    if (strlen(name)==1 && name[0]=='/') return -1;

    char *pre, *post;
    char *tmp = balloc(strlen(name)+1);
    strcpy(tmp, name);
    pre = alldir(tmp);
    post = filename(tmp);

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

fsops_t ext2_ops = {
    .init = ext2_init,
    .lookup = ext2_lookup,
    .close = ext2_close,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rm,
    .unlink = ext2_rm,
    .create = ext2_create,
};

/* ===== Inode API ====== */

int ext2_inode_open(file_t *file, int flags){
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
            file->inode->size = inode->size = update_size;
            dev->ops->write(dev, TABLE(inode->index), inode, INODE_BYTES);
        }
    }

    return size;
}

int ext2_inode_link(file_t *file, const char *name){
    ext2_inode_t* inode = file->inode->fs_inode;
    device_t* dev = file->inode->fs->dev;

    char *pre, *post;
    pre = alldir(name);
    post = filename(name);

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
