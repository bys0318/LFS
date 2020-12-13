// TO DO:
// 1. adding time --> Done
// 2. adding mode check
// 3. adding nlink change
// 4. adding file operation --> Done
// 5. reboot
// 6. free the malloc --> Done
// 7. directory should see the subfile's attribute of 
#include "metadata.h"
#include <time.h>

struct LFS* lfs;

static int min(int a, int b) {return a < b ? a : b;}

int yrx_init_lfs(struct LFS** lfs) {
    *lfs = malloc(sizeof(struct LFS));
    (*lfs)->nextblock = 1;
    (*lfs)->transaction = 0;
    (*lfs)->buffersize = 0;
    strcpy((*lfs)->filename, "LFS");
    //memset((*lfs)->superblock.inodemap, -1, INODE_MAP_SIZE * sizeof(int));
    for (int i = 0; i < INODE_MAP_SIZE; ++i) (*lfs)->superblock.inodemap[i] = -1;
    struct INode* root = malloc(sizeof(struct INode));
    yrx_createinode(*lfs, 0, root);
    root->isdir = 1;
    //root->block_num = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    for (int i = 0; i < MAX_FILE_NUM; i++) dir->id[i] = -1;
    strcpy(dir->filenames[0], "."); // TODO
    dir->id[0] = root->id;
    root->addr[0] = (*lfs)->nextblock + (*lfs)->buffersize;
    yrx_writedir(*lfs, 0, dir);
    // (*lfs)->superblock.inodemap[root->id] = (*lfs)->nextblock;
    yrx_writeinode(*lfs, 0, root);
    yrx_writebuffertodisk(*lfs);
    free(root);
    free(dir);
    return 0;
}

int yrx_init_indir(struct Indirection* indir) {
    indir->indir = -1;
    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) indir->blocks[i] = -1;
    return 0;
}

int yrx_check_access(struct INode* node, uid_t uid) {
    return 1;
    //unsigned int user = node->mode / 64, group = (node->mode / 8) % 8, all = node->mode % 8;
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
    // inode update, TO DO
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
            if (yrx_check_access(node, uid) == 1) { // has the permission
                // from the directory to find the next one
                char nextname[MAX_FILENAME];
                int tmp = 0;
                while (path[index] != '/' && index < end && tmp < MAX_FILENAME) nextname[tmp++] = path[index++]; // trunc the name
                nextname[tmp] = '\0';
                while (path[index] != '/' && index < end) index ++; // find the next '/'
                if (node->isdir == 0) return 0;
                struct Directory dir;
                yrx_readdir(lfs, tid, node->addr[0], dir);
                for (int i = 0; i < MAX_FILE_NUM; ++i) {
                    if (strcmp(nextname, dir.filenames[i]) == 0) {
                        found = yrx_readinode(lfs, tid, dir.id[i], node);
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

int yrx_readfilefrominode(struct LFS* lfs, int tid, char* file, struct INode* node, size_t size, off_t offset) { //TODO: double check
    int index = 0;
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + size) / BLOCK_SIZE + 1;
    int end = POINTER_PER_INODE, addr = node->indir;
    struct Indirection* indir = malloc(sizeof(struct Indirection));
    char* block = malloc(BLOCK_SIZE);
    while (end <= start_block_num) {
        // read from buffer or disk
        if (addr > lfs->nextblock) yrx_readblockfrombuffer(lfs, addr - lfs->nextblock, indir, sizeof(struct Indirection), 1);
        else yrx_readblockfromdisk(lfs, addr, indir, sizeof(struct Indirection), 1);
        addr = indir->indir;
        start_block_num -= end;
        end_block_num -= end;
        end = BLOCK_PER_INDIRECTION;
    }
    if (end_block_num < end) {
        for (int i = start_block_num; i <= end_block_num; ++i) {
            int address = indir->blocks[i];
            if (address >= lfs->nextblock) yrx_readblockfrombuffer(lfs, address - lfs->nextblock, block, BLOCK_SIZE, 1);
            else yrx_readblockfromdisk(lfs, address, block, BLOCK_SIZE, 1);
            memcpy(file+index, block, BLOCK_SIZE);
            index ++;
        }
    }
    else {
        for (int i = start_block_num; i <= min(end_block_num + 1, BLOCK_PER_INDIRECTION); ++i) {
            int address = indir->blocks[i];
            if (address >= lfs->nextblock) yrx_readblockfrombuffer(lfs, address - lfs->nextblock, block, BLOCK_SIZE, 1);
            else yrx_readblockfromdisk(lfs, address, block, BLOCK_SIZE, 1);
            memcpy(file+index, block, BLOCK_SIZE);
            index ++;
        }
        while (end_block_num >= BLOCK_PER_INDIRECTION) {
            if (addr > lfs->nextblock) yrx_readblockfrombuffer(lfs, addr - lfs->nextblock, indir, sizeof(struct Indirection), 1);
            else yrx_readblockfromdisk(lfs, addr, indir, sizeof(struct Indirection), 1);
            addr = indir->indir;
            end_block_num -= end;
            for (int i = 0; i < min(end_block_num + 1, BLOCK_PER_INDIRECTION); ++i) {
                int address = indir->blocks[i];
                if (address >= lfs->nextblock) yrx_readblockfrombuffer(lfs, address - lfs->nextblock, block, BLOCK_SIZE, 1);
                else yrx_readblockfromdisk(lfs, address, block, BLOCK_SIZE, 1);
                memcpy(file+index, block, BLOCK_SIZE);
                index ++;
            }
        }
    }
    free(indir);
    free(block);
    return 0;
}

int yrx_readdirfrominode(struct LFS* lfs, int tid, struct INode* node, struct Directory* dir) {
    return yrx_readdir(lfs, tid, node->addr[0], dir); 
}

int yrx_readindirectionblock(struct LFS* lfs, int tid, int addr, struct Indirection* indir) {
    if (addr >= lfs->nextblock) yrx_readblockfrombuffer(lfs, addr, indir, sizeof(struct Indirection), 1);
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
    return found;
}

int yrx_linkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename, struct INode* node) { // TODO: wtf?
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(filename, dir->filenames[i]) == 0) {
            dir->id[i] = node->id;
            found = 1;
            break;
        }
    }
    free(dir);
    return found;
}

int yrx_unlinkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename) { // TODO: wtf?
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(dir->filenames[i], filename) == 0) {
            dir->id[i] = -1;
            found = 1;
            break;
        }
    }
    free(dir);
    return found;
}

int yrx_createinode(struct LFS* lfs, int tid, struct INode* node) {
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
            node->gid = 0;
            node->uid = 0;
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
int yrx_createdir(struct LFS* lfs, int tid, struct INode* fnode, const char* filename) {  // TODO: relation with createfile
    struct Directory* fdir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, tid, fnode, fdir); // read the parent dir
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (fdir->id[i] == -1) {
            // initial the directory
            struct Directory* dir = malloc(sizeof(struct Directory)); // assign the space
            strcpy(dir->filenames[0], ".."); // add parent pointer
            strcpy(dir->filenames[1], ".");
            dir->id[0] = fnode->id;
            // create a inode for it 
            struct INode* node = malloc(sizeof(struct INode));
            yrx_createinode(lfs, tid, node); 
            node->addr[0] = lfs->nextblock + lfs->buffersize;
            node->nlink = 0;
            node->isdir = 1;
            yrx_writedir(lfs, tid, dir);
            // update parent inode
            fnode->child_num ++;
            // update parent directory
            strcpy(fdir->filenames[i], filename);
            fdir->id[i] = node->id;
            dir->id[i] = node->id;
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
                if (node.child_num > 0) return 2; // still have child, cannot delete it
                else {
                    lfs->superblock.inodemap[node.id] = -1; // delete the node
                    fdir.filenames[i][0] = '\0';  
                    fdir.id[i] = -1;
                    fnode->child_num --;
                    return 1;
                }
            }
            else return 0;
        }
    }
    return 0;
}

// write file:
// clear buffer
// write to block directly
// update the inode
int yrx_writefile(struct LFS* lfs, int tid, const char* file, struct INode* node, size_t size, off_t offset) { // TODO
    yrx_writebuffertodisk(lfs);
    // update the inode
    // [start, end]
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + size) / BLOCK_SIZE + 1;
    if (end_block_num < POINTER_PER_INODE) { // no need of indirection
        for (int i = start_block_num; i <= end_block_num; ++i) node->addr[i] = lfs->nextblock + i - start_block_num;
    }
    else {
        if (start_block_num < 10) {
            for (int i = start_block_num; i < POINTER_PER_INODE; ++i) node->addr[i] = lfs->nextblock + i - start_block_num;
            yrx_writeinode(lfs, tid, node);
        }
        else {
            int index = 0;
            int maxblock = POINTER_PER_INODE - 1;
            int address = node->indir;
            int base = lfs->nextblock + (size - 1) / BLOCK_SIZE + 1;
            node->indir = base + lfs->buffersize + 1; // one is the space of inode
            yrx_writeinode(lfs, tid, node);
            struct Indirection* indir = malloc(sizeof(struct Indirection));
            while (maxblock < start_block_num) {
                if (address == -1) yrx_init_indir(indir);
                else yrx_readindirectionblock(lfs, tid, address, indir);
                address = indir->indir;
                maxblock += BLOCK_PER_INDIRECTION;
            }
            if (end_block_num <= maxblock) {
                for (int i = start_block_num; i <= end_block_num; ++i)
                    indir->blocks[(i-POINTER_PER_INODE)%BLOCK_PER_INDIRECTION] = lfs->nextblock + index++;
                yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
            }
            else {
                for (int i = start_block_num; i <= maxblock; ++i) 
                    indir->blocks[(i-POINTER_PER_INODE)%BLOCK_PER_INDIRECTION] = lfs->nextblock + index++;
                while (maxblock < end_block_num) {
                    indir->indir = base + lfs->buffersize + 1;
                    yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
                    if (address == -1) yrx_init_indir(indir);
                    else yrx_readindirectionblock(lfs, tid, address, indir);
                    address = indir->indir;
                    maxblock += BLOCK_PER_INDIRECTION;
                    if (maxblock > end_block_num) maxblock = end_block_num;
                    for (int i = 0; i <= (maxblock - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) 
                        indir->blocks[i] = lfs->nextblock + index++;
                }
                yrx_writeblocktobuffer(lfs, indir, sizeof(struct Indirection));
            }
            free(indir);
        }
    }
    yrx_writeblocktodisk(lfs, lfs->nextblock, file, size, 1);
    yrx_writebuffertodisk(lfs);
    return 0;
}

int yrx_begintransaction(struct LFS* lfs) {
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
    fprintf(file, "Size: %lld\nPosition of block in log: %d\nMode: %d\nNlink: %d\nUid: %d\n\
        Gid: %d\nNblock: %d\nPosition of indirection block: %d\nIsdirectory: %dAccess time: %d\n\
        Modify time: %d\nCreate time: %d\n", node->size, node->block_num, node->mode,\
        node->nlink, node->uid, node->gid, node->blocks, node->indir, node->isdir, node->atime,\
        node->mtime, node->ctime);
    for (int i = 0; i < POINTER_PER_INODE; i++){
        fprintf(file, "Pointer%d: %d\n", i, node->addr[i]);
    }
    fprintf(file, "\n");
    return 0;
}

int SuperBlocktoString(FILE *file, struct SuperBlock* superblock){
    fprintf(file, "Printing Superblock\n");
    fprintf(file, "Free_block_num: %d\nFree inode num: %d\n", superblock->free_block_num, superblock->free_inode_num);
    fprintf(file, "Inode map { ");
    for (int i = 0; i < INODE_MAP_SIZE; i++){
        fprintf(file, "%d:%d ", i, superblock->inodemap[i]);
    }
    fprintf(file, "}\n\n");
    return 0;
}

int block_dump(struct LFS* lfs){
    FILE *file = fopen("block.txt", "a");
    SuperBlocktoString(file, &lfs->superblock);
    for (int i = 0; i < INODE_MAP_SIZE; i++){
        if (lfs->superblock.inodemap[i] != -1){
            struct INode* node = malloc(sizeof(struct INode));
            yrx_readinode(lfs, 0, lfs->superblock.inodemap[i], node);
            INodetoString(file, node);
            free(node);
        }
    }
    return 0;
}