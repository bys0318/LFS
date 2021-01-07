// TO DO:
// 1. adding time --> Done
// 2. adding mode check --> Done
// 3. adding nlink change --> Done
// 4. adding file operation --> 
// 5. reboot
// 6. free the malloc --> Done
// 7. directory should see the subfile's attribute of 
#include "metadata.h"
#include <time.h>
#include<fcntl.h>
#include <assert.h>

struct LFS* lfs;

static int min(int a, int b) {return a < b ? a : b;}
static int max(int a, int b) {return a > b ? a : b;}

int yrx_init_lfs(struct LFS** lfs) {
    *lfs = malloc(sizeof(struct LFS));
    (*lfs)->nextblock = 1;
    (*lfs)->transaction = 0;
    (*lfs)->buffersize = 0;
    strcpy((*lfs)->filename, "/mnt/c/Users/'Yang Ruixiao'/Desktop/LFS");
    //memset((*lfs)->superblock.inodemap, -1, INODE_MAP_SIZE * sizeof(int));
    for (int i = 0; i < INODE_MAP_SIZE; ++i) (*lfs)->superblock.inodemap[i] = -1;
    struct INode root;
    yrx_createinode(*lfs, 0, &root, 16895, 0, 0);
    root.isdir = 1;
    //root->block_num = 0;
    struct Directory dir;
    for (int i = 0; i < MAX_FILE_NUM; i++) dir.id[i] = -1;
    strcpy(dir.filenames[0], "."); // TODO
    dir.id[0] = root.id;
    root.addr[0] = (*lfs)->nextblock + (*lfs)->buffersize;
    yrx_writedir(*lfs, 0, &dir);
    // (*lfs)->superblock.inodemap[root->id] = (*lfs)->nextblock;
    yrx_writeinode(*lfs, 0, &root);
    yrx_writebuffertodisk(*lfs);
    return 0;
}

int yrx_init_indir(struct Indirection* indir) {
    indir->indir = -1;
    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) indir->blocks[i] = -1;
    return 0;
}

int yrx_check_access(struct INode* node, uid_t uid) {
    if (uid == node->uid) return node->mode & S_IXUSR;
    else if (checkusergroup(uid, node->gid)) return node->mode & S_IXGRP;
    else return node->mode & S_IXOTH;
}

int yrx_readblockfromdisk(struct LFS* lfs, int block_num, void* block, int size, int time) {
    FILE* file = fopen(lfs->filename, "r");
    if (file == NULL) {
        perror("Error: ");
        return(-1);
    }
    fseek(file, block_num * BLOCK_SIZE, SEEK_SET);
    fread(block, size, time, file);
    fclose(file);
    return 0;
}

int yrx_readblockfrombuffer(struct LFS* lfs, int block_num, void* block, int size, int time) {
    memcpy(block ,lfs->buffer+block_num * BLOCK_SIZE, size);
    return 0;
}

int yrx_writeblocktodisk(struct LFS* lfs, int block_num, const void* block, int size, int time) {
    FILE* file = fopen(lfs->filename, "r+");
    if (file == NULL) {
        perror("Error: ");
        return(-1);
    }
    //fseek(file, block_num * BLOCK_SIZE, SEEK_SET);
    //since it is append only, go to end of the file 
    fseek(file, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(block, size, time, file); 
    fclose(file);
    return 0;
}

int yrx_writeblocktobuffer(struct LFS* lfs, const void* block, int size) {
    memcpy(lfs->buffer+lfs->buffersize * BLOCK_SIZE, block, size);
    lfs->buffersize += (size - 1) / BLOCK_SIZE + 1;
    return 0;
}

int yrx_writebuffertodisk(struct LFS* lfs) {
    if (lfs->buffersize == 0) return 0;
    yrx_writeblocktodisk(lfs, lfs->nextblock, lfs->buffer, lfs->buffersize * BLOCK_SIZE, 1);
    lfs->nextblock += lfs->buffersize;
    lfs->buffersize = 0;
    return 0;
}

int yrx_readblock(struct LFS* lfs, int tid, int block_num, void* block, int size, int time) {
    if (block_num < 0) return 0;
    else if (block_num < lfs->nextblock) yrx_readblockfromdisk(lfs, block_num, block, size, time);
    else yrx_readblockfrombuffer(lfs, block_num - lfs->nextblock, block, size, time);
}

int yrx_readinode(struct LFS* lfs, int tid, int id, struct INode* node) {
    if (id < 0) return 0;
    int block_num = lfs->superblock.inodemap[id];
    if (lfs->superblock.inodemap[id] < 0) return 0; 
    if (block_num < lfs->nextblock) yrx_readblockfromdisk(lfs, block_num, node, sizeof(struct INode), 1);
    else yrx_readblockfrombuffer(lfs, block_num-lfs->nextblock, node, sizeof(struct INode), 1);
    node->atime = time(NULL);
    return 1;
}

int yrx_writeinode(struct LFS* lfs, int tid, struct INode* node) {
    lfs->superblock.inodemap[node->id] = lfs->nextblock + lfs->buffersize;
    node->mtime = time(NULL);
    yrx_writeblocktobuffer(lfs, node, sizeof(struct INode));
    return 0;
}

int yrx_readdir(struct LFS* lfs, int tid, int block_num, struct Directory* dir) {
    if (block_num < lfs->nextblock) yrx_readblockfromdisk(lfs, block_num, dir, sizeof(struct Directory), 1);
    else yrx_readblockfrombuffer(lfs, block_num-lfs->nextblock, dir, sizeof(struct Directory), 1);
    return 0;
}

int yrx_writedir(struct LFS* lfs, int tid, struct Directory* dir) {
    yrx_writeblocktobuffer(lfs, dir, sizeof(struct Directory));
    return 0;
}

int yrx_readinodefrompath(struct LFS* lfs, int tid, const char* path, struct INode* node, uid_t uid) {
    int found = 0;
    if (yrx_readinode(lfs, tid, 0, node) == 0) return 0; 
    printf("%d %d %d\n",node->addr[0], lfs->nextblock, node->id);
    if (strcmp(path, "/") == 0) { // read the root inode
        found = 1; // root inode is always the 0th
    }
    else { // read other inode
        char filename[MAX_FILENAME];
        int index = strlen(path) - 1;
        while (index > 0 && path[index] != '/') index--; // here may have problem, not know unequal is appliable, TO CHECK
        strncpy(filename, path+index+1, MAX_FILENAME);
        index = 0;
        int end = strlen(path);
        while (found == 0 && index < end) {
            index ++;
            if (yrx_check_access(node, uid)) { // has the permission
                // from the directory to find the next one
                char nextname[MAX_FILENAME];
                int tmp = 0;
                while (path[index] != '/' && index < end && tmp < MAX_FILENAME) nextname[tmp++] = path[index++]; // trunc the name
                nextname[tmp] = '\0';
                while (path[index] != '/' && index < end) index ++; // find the next '/'
                if (node->isdir == 0) return 0;
                struct Directory dir;
                yrx_readdir(lfs, tid, node->addr[0], &dir);
                for (int i = 0; i < MAX_FILE_NUM; ++i) {
                    if (strcmp(nextname, dir.filenames[i]) == 0) {
                        found = yrx_readinode(lfs, tid, dir.id[i], node);
                        yrx_writeinode(lfs, tid, node);
                        break;
                    }
                }
                if (found == 1) {
                    if (strcmp(nextname, filename) == 0) break;
                    else found = 0;
                }
                else return 0;
            }
            else return 2;
        }
    }
    return found;
}

int yrx_readfilefrominode(struct LFS* lfs, int tid, char* file, struct INode* node, size_t size, off_t offset) { 
    //yrx_simpleread(lfs, tid, file, node, size, offset);
    //return 0;
    int index = 0;
    int true_size = min(size, node->size - offset);
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + true_size) / BLOCK_SIZE;
    int end = POINTER_PER_INODE - 1, addr = node->indir;
    struct Indirection* indir = malloc(sizeof(struct Indirection));
    char* buffer = malloc(BLOCK_SIZE * (true_size / BLOCK_SIZE + 1));
    if (end_block_num < POINTER_PER_INODE) {// no indirection
        for (int i = start_block_num; i <= end_block_num; ++i) {
            yrx_readblock(lfs, tid, node->addr[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
            index ++;
        }
        memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
    }
    else if (start_block_num < POINTER_PER_INODE) {
        for (int i = start_block_num; i < POINTER_PER_INODE; ++i) {
            yrx_readblock(lfs, tid, node->addr[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
            index++;
        }
        while (end < end_block_num) {
            if (addr < 0) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, tid, addr, indir);
            addr = indir->indir;
            end += BLOCK_PER_INDIRECTION;
            if (end < end_block_num)
                for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                    yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
                    index ++;
                }
        }
        for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
            yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
            index ++;
        }
        memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
    }
    else {
        while (end < start_block_num) {
            if (addr < 0) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, tid, addr, indir);
            end += BLOCK_PER_INDIRECTION;
            addr = indir->indir;
        }
        if (end_block_num <= end) {
            for (int i = (start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
                yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
                index ++;
            }
            memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
        }
        else {
            for (int i = (start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; i < BLOCK_PER_INDIRECTION; ++i) {
                yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
                index ++;
            }
            while (end < end_block_num) {
                if (addr < 0) yrx_init_indir(indir);
                else yrx_readindirectionblock(lfs, tid, addr, indir);
                addr = indir->indir;
                end += BLOCK_PER_INDIRECTION;
                if (end < end_block_num)
                    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                        yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
                        index ++;
                    }
            }
            for (int i = 0; i < (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
                yrx_readblock(lfs, tid, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE, 1);
                index ++;
            }
            memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
        }
    }
    free(buffer);
    free(indir);
    return 0;
}

int yrx_readdirfrominode(struct LFS* lfs, int tid, struct INode* node, struct Directory* dir) {
    return yrx_readdir(lfs, tid, node->addr[0], dir); 
}

int yrx_readindirectionblock(struct LFS* lfs, int tid, int addr, struct Indirection* indir) {
    if (addr >= lfs->nextblock) yrx_readblockfrombuffer(lfs, addr - lfs->nextblock, indir, sizeof(struct Indirection), 1);
    else yrx_readblockfromdisk(lfs, addr, indir, sizeof(struct Indirection), 1);
    return 0;
}

int yrx_renamefile(struct LFS* lfs, int tid, struct INode* fnode, const char* originname, const char* newname) {// TO DO: check
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(originname, dir->filenames[i]) == 0) {
            strcpy(dir->filenames[i], newname);
            found = 1;
            break;
        }
    }
    free(dir);
    fnode->addr[0] = lfs->nextblock + lfs->buffersize;
    yrx_writedir(lfs, tid, dir);
    yrx_writeinode(lfs, tid, fnode);
    return found;
}

int yrx_linkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename, struct INode* node) { // TODO: wtf?
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (dir->id[i] == -1) {
            strcpy(dir->filenames[i], filename);
            dir->id[i] = node->id;
            node->nlink ++;
            found = 1;
            fnode->child_num ++;
            break;
        }
    }
    fnode->addr[0] = lfs->nextblock + lfs->buffersize;
    yrx_writedir(lfs, tid, dir);
    yrx_writeinode(lfs, tid, node);
    yrx_writeinode(lfs, tid, fnode);
    free(dir);
    return found;
}

int yrx_unlinkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename) { // TODO: wtf?
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(dir->filenames[i], filename) == 0) {
            struct INode node;
            yrx_readinode(lfs, tid, dir->id[i], &node);
            dir->id[i] = -1;
            found = 1;
            node.nlink --;
            if (node.nlink == 0) lfs->superblock.inodemap[node.id] = -1; // release the inode
            yrx_writeinode(lfs, tid, &node);
            break;
        }
    }
    fnode->addr[0] = lfs->nextblock + lfs->buffersize;
    yrx_writedir(lfs, tid, dir);
    yrx_writeinode(lfs, tid, fnode);
    free(dir);
    return found;
}

int yrx_createinode(struct LFS* lfs, int tid, struct INode* node, mode_t mode, uid_t uid, gid_t gid) {
    //node = malloc(sizeof(struct INode));
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (lfs->superblock.inodemap[i] == -1) {
            node->id = i;
            lfs->superblock.inodemap[i] = -2;
            node->ctime = time(NULL);
            node->atime = node->ctime;
            node->mtime = node->atime;
            node->blocks = 0;
            node->indir = -1;
            node->block_num = -1;
            node->isdir = 0; // default file
            node->child_num = 0;
            node->size = 0;
            node->nlink = 0;
            node->mode = mode;
            node->gid = gid;
            node->uid = uid;
            for (int i = 0; i < POINTER_PER_INODE; ++i) node->addr[i] = -1;
            break;
        }
    }
    return 0;
}

// create a directory:
// add the name in parent directory
// create a inode for it 
// initial the directory
// update parent inode
int yrx_createdir(struct LFS* lfs, int tid, struct INode* fnode, const char* filename, mode_t mode, uid_t uid, gid_t gid) {  // TODO: relation with createfile
    struct Directory* fdir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, fdir); // read the parent dir
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (fdir->id[i] == -1) {
            // initial the directory
            struct Directory* dir = malloc(sizeof(struct Directory)); // assign the space
            for (int i = 0; i < MAX_FILE_NUM; ++i) dir->id[i] = -1;
            strcpy(dir->filenames[0], ".."); // add parent pointer
            strcpy(dir->filenames[1], ".");
            dir->id[0] = fnode->id;
            // create a inode for it 
            struct INode* node = malloc(sizeof(struct INode));
            yrx_createinode(lfs, tid, node, mode, uid, gid); 
            node->addr[0] = lfs->nextblock + lfs->buffersize;
            node->nlink = 1;
            node->isdir = 1;
            dir->id[1] = node->id;
            yrx_writedir(lfs, tid, dir);
            // update parent inode
            fnode->child_num ++;
            // update parent directory
            strcpy(fdir->filenames[i], filename);
            fdir->id[i] = node->id;
            yrx_writeinode(lfs, tid, node);
            fnode->addr[0] = lfs->nextblock + lfs->buffersize;            
            yrx_writedir(lfs, tid, fdir);
            yrx_writeinode(lfs, tid, fnode);
            printf("Create Finish\n");
            free(fdir);
            free(node);
            free(dir);
            return 1;
        }
    }
    free(fdir);
    yrx_writeinode(lfs, tid, fnode);
    return 0;
}

// delete a directory:
// 
int yrx_deletedir(struct LFS* lfs, int tid, struct INode* fnode, const char* filename) {
    struct Directory fdir;
    yrx_readdirfrominode(lfs, tid, fnode, &fdir); // read the parent dir
    struct INode node;
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (strcmp(fdir.filenames[i], filename) == 0) {
            yrx_readinode(lfs, tid, fdir.id[i], &node); // read the dir's inode
            if (node.isdir == 1) {
                if (node.child_num > 0) {
                    yrx_writeinode(lfs, tid, &node);
                    yrx_writeinode(lfs, tid, fnode);
                    return 2; // still have child, cannot delete it
                }
                else {
                    lfs->superblock.inodemap[node.id] = -1; // delete the node
                    fdir.filenames[i][0] = '\0';  
                    fdir.id[i] = -1;
                    fnode->child_num --;
                    fnode->addr[0] = lfs->nextblock + lfs->buffersize;
                    yrx_writedir(lfs, tid, &fdir);
                    yrx_writeinode(lfs, tid, fnode);
                    return 1;
                }
            }
            else {
                yrx_writeinode(lfs, tid, &node);
                yrx_writeinode(lfs, tid, fnode);
                return 0;
            }
        }
    }
    yrx_writeinode(lfs, tid, fnode);
    return 0;
}


int yrx_simpleread(struct LFS* lfs, int tid, char* file, struct INode* node, size_t size, off_t offset){
    char* buffer = malloc(POINTER_PER_INODE * BLOCK_SIZE);
    for (int i = 0; i < POINTER_PER_INODE; ++i) {
        if (node->addr[i] == -1) break;
        yrx_readblock(lfs, tid, node->addr[i], buffer + i * BLOCK_SIZE, BLOCK_SIZE, 1);
    }
    memcpy(file, buffer + offset, size);
    return 0;
}
int yrx_simplewrite(struct LFS* lfs, int tid, const char* file, struct INode* node, size_t size, off_t offset){
    char* buffer = malloc(POINTER_PER_INODE * BLOCK_SIZE);
    for (int i = 0; i < POINTER_PER_INODE; ++i) {
        if (node->addr[i] == -1) break;
        yrx_readblock(lfs, tid, node->addr[i], buffer + i * BLOCK_SIZE, BLOCK_SIZE, 1);
    }
    memcpy(buffer + offset, file, size);
    int start = offset / BLOCK_SIZE, end = (offset + size) / BLOCK_SIZE;
    for (int i = start; i <= end; ++i) {
        node->addr[i] = lfs->nextblock + lfs->buffersize;
        yrx_writeblocktobuffer(lfs, buffer + i * BLOCK_SIZE, BLOCK_SIZE);
    }
    node->size = max(node->size, offset + size);
    yrx_writeinode(lfs, tid, node);
}
// write file:
// clear buffer
// write to block directly
// update the inode
int yrx_writefile(struct LFS* lfs, int tid, const char* file, struct INode* node, size_t size, off_t offset) { 
    // yrx_simplewrite(lfs, tid, file, node, size, offset);
    // return 0;
    yrx_writebuffertodisk(lfs);
    // update the inode
    // [start, end]
    char* buffer = malloc(size + 2 * BLOCK_SIZE); // two extra blocks for begin and end
    int buffersize = 0;
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + size) / BLOCK_SIZE;
    int maxend = POINTER_PER_INODE - 1;
    struct Indirection* indir = malloc(sizeof(struct Indirection));
    int address = node->indir;
    int realaddr = lfs->nextblock;
    int startaddr = lfs->nextblock;
    lfs->nextblock += end_block_num - start_block_num + 1;
    int base = realaddr + end_block_num - start_block_num + 2; // extra one is for node
    node->size = max(node->size, offset + size);
    node->blocks = node->size / BLOCK_SIZE + 1;
    if (end_block_num < POINTER_PER_INODE) { // no need of indirection
        for (int i = start_block_num; i <= end_block_num; ++i) {
            if (node->addr[i] == -1) break;
            if (i == start_block_num || i == end_block_num) yrx_readblock(lfs, tid, node->addr[i], buffer + (i - start_block_num) * BLOCK_SIZE, BLOCK_SIZE, 1);
        }
        for (int i = start_block_num; i <= end_block_num; ++i) node->addr[i] = startaddr + buffersize++;
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writeblocktodisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE, 1);
        yrx_writeinode(lfs, tid, node);
        yrx_writebuffertodisk(lfs);
    }
    else if (start_block_num < POINTER_PER_INODE) {
        yrx_readblock(lfs, tid, node->addr[start_block_num], buffer, BLOCK_SIZE, 1);
        for (int i = start_block_num; i < POINTER_PER_INODE; ++i) {
            node->addr[i] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        node->indir = base;
        base ++;
        yrx_writeinode(lfs, tid, node);
        while (maxend < end_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, tid, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (end_block_num <= maxend) break;
            else indir->indir = base ++;
            for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                indir->blocks[i] = realaddr;
                realaddr ++;
                buffersize ++;
            }
            yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        }
        yrx_readblock(lfs, tid, indir->blocks[(end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer + (end_block_num - start_block_num) * BLOCK_SIZE, BLOCK_SIZE, 1);
        for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
            indir->blocks[i] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writeblocktodisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE, 1);
        yrx_writebuffertodisk(lfs);
    }
    else {
        node->indir = base++;
        yrx_writeinode(lfs, tid, node);
        while (maxend < start_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, tid, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (maxend < end_block_num) indir->indir = base++;
            if (maxend < start_block_num) yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        }
        yrx_readblock(lfs, tid, indir->blocks[(start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer, BLOCK_SIZE, 1);
        for (int i = start_block_num; i <= maxend; ++i) {
            indir->blocks[(i-POINTER_PER_INODE)%BLOCK_PER_INDIRECTION] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        // deal the first one 
        // Now indirection contains the start block
        while (maxend < end_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, tid, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (end_block_num <= maxend) break;
            else indir->indir = base ++;
            for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                indir->blocks[i] = realaddr ++;
                buffersize ++;
            }
            yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        }
        yrx_readblock(lfs, tid, indir->blocks[(end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer + end_block_num - start_block_num, BLOCK_SIZE, 1);
        for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
            indir->blocks[i] = realaddr++;
            buffersize ++;
        }
        yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writeblocktodisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE, 1);
        yrx_writebuffertodisk(lfs);
    }
    free(buffer);
    free(indir);
    return 0;
}

static void sleep_ms(unsigned int secs)
{
    struct timeval tval;
    tval.tv_sec=secs/1000;
    tval.tv_usec=(secs*1000)%1000000;
    select(0,NULL,NULL,NULL,&tval);
}

int yrx_begintransaction(struct LFS* lfs) {
    if (lfs->nextblock > 1024 * 49) clean(1);
    if (lfs->transaction == 0) {
        lfs->transaction = 1;
        return 1;
    }
    else return 0;
}

int yrx_endtransaction(struct LFS* lfs, int tid) {
    if (lfs->transaction == 1) {
        yrx_writebuffertodisk(lfs);
        lfs->transaction = 0;
        return 1;
    }
    else return 0;
}

int INodetoString(FILE *file, struct INode* node){
    fprintf(file, "Printing Inode %d\n", node->id);
    fprintf(file, "Size: %lld\nPosition of block in log: %d\nMode: %d\nNlink: %d\nUid: %d\nGid: %d\nNblock: %d\nPosition of indirection block: %d\nIsdirectory: %d\nAccess time: %d\nModify time: %d\nCreate time: %d\n", node->size, node->block_num, node->mode, node->nlink, node->uid, node->gid, node->blocks, node->indir, node->isdir, node->atime, node->mtime, node->ctime);
    for (int i = 0; i < POINTER_PER_INODE; i++){
        fprintf(file, "Pointer%d: %d\n", i, node->addr[i]);
    }
    fprintf(file, "\n");
    return 0;
}

int SuperBlocktoString(FILE *file, struct SuperBlock* superblock){
    fprintf(file, "Printing Superblock\n");
    fprintf(file, "Inode map { ");
    for (int i = 0; i < INODE_MAP_SIZE; i++){
        fprintf(file, "%d:%d ", i, superblock->inodemap[i]);
    }
    fprintf(file, "}\n\n");
    return 0;
}

int block_dump(struct LFS* lfs){ return 0;
    FILE *file = fopen("/home/bys/block.txt", "a");
    fprintf(file, "\nBlock dump START!\n\n");
    SuperBlocktoString(file, &lfs->superblock);
    for (int i = 0; i < INODE_MAP_SIZE; i++){
        if (lfs->superblock.inodemap[i] != -1){
            struct INode* node = malloc(sizeof(struct INode));
            yrx_readinode(lfs, 0, i, node);
            INodetoString(file, node);
            free(node);
        }
    }
    fprintf(file, "Block dump END!\n");
    fclose(file);
    return 0;
}


int clean(int tid) { // INode did not update file size
    char* buffer = malloc(lfs->nextblock * BLOCK_SIZE);
    struct INode node;
    int blockIndex = 0; // TO DO: adjust later
    int lastfilesize = 0;
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (lfs->superblock.inodemap[i] > -1) yrx_readinode(lfs, tid, i, &node);
        if (node.isdir == 0) { // is file
            int base = node.blocks + blockIndex;
            yrx_readfilefrominode(lfs, tid, buffer + blockIndex * BLOCK_SIZE, &node, node.size, 0);
            if (node.blocks <= POINTER_PER_INODE) for (int i = 0; i < node.blocks; ++i) node.addr[i] = blockIndex++;
            else {
                for (int i = 0; i < POINTER_PER_INODE; ++i) node.addr[i] = blockIndex++;
                int addr = node.indir;
                node.indir = base++;
                while (addr > -1) {
                    struct Indirection indir;
                    yrx_readindirectionblock(lfs, tid, addr, &indir);
                    addr = indir.indir;
                    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                        if (indir.blocks[i] > -1 ) indir.blocks[i] = blockIndex++;
                        else break;
                    }
                    if (addr > -1) indir.indir = base;
                    memcpy(buffer + base * BLOCK_SIZE, &indir, sizeof(struct Indirection));
                    base++;
                }
                blockIndex = base;
            }
        }
        else { // is directory
            struct Directory dir;
            yrx_readdirfrominode(lfs, tid, &node, &dir);
            node.addr[0] = blockIndex;
            memcpy(buffer + blockIndex * BLOCK_SIZE, &dir, sizeof(struct Directory));
            blockIndex++;
        }
        lfs->superblock.inodemap[i] = blockIndex;
        memcpy(buffer + blockIndex * BLOCK_SIZE, &node, sizeof(struct INode));
        blockIndex++;
    }
    lfs->nextblock = 0; // TO DO: adjust later
    yrx_writeblocktodisk(lfs, lfs->nextblock, buffer, (blockIndex - 1) * BLOCK_SIZE + lastfilesize, 1);
    lfs->nextblock += blockIndex;
    free(buffer);
}