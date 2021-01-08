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
static void sleep_ms(unsigned int secs) {
    struct timeval tval;
    tval.tv_sec=secs/1000;
    tval.tv_usec=(secs*1000)%1000000;
    select(0,NULL,NULL,NULL,&tval);
}

int yrx_init_lfs(struct LFS** lfs, int reboot) {
    *lfs = malloc(sizeof(struct LFS));
    strcpy((*lfs)->filename, "/home/bys/LFS");
    (*lfs)->cleaning = 0;
    (*lfs)->nt=0;
	(*lfs)->mv=0;
	(*lfs)->cache=calloc(CACHE_SIZE,sizeof(struct node));
	fclose(fopen("/home/bys/log","w"));
	for(int f1=0;f1<CACHE_SIZE;f1++)
		(*lfs)->cache[f1].num=-2;
    if (reboot == 1){ // reboot on last version
        FILE* file = fopen((*lfs)->filename, "r");
        if (file == NULL) {
            perror("Error: ");
            return(-1);
        }
        fseek(file, 0, SEEK_SET);
        fread(&((*lfs)->superblock), sizeof(struct SuperBlock), 1, file);
        fclose(file);
        pthread_mutex_init(&(*lfs)->lock, NULL);
        (*lfs)->nextblock = (*lfs)->superblock.nextblock;
        for (int i = 0; i < INODE_MAP_SIZE; ++i) pthread_rwlock_init(&((*lfs)->superblock.lockmap[i]), NULL);
        return 0;
    }
    (*lfs)->nextblock = 100;
    for (int i = 0; i < INODE_MAP_SIZE; ++i) (*lfs)->superblock.inodemap[i] = -1;
    pthread_mutex_init(&(*lfs)->lock, NULL);
    // create inode for root
    struct INode root;
    yrx_createinode(*lfs, &root, 16895, 0, 0);
    root.isdir = 1;
    // init root file (directory)
    struct Directory dir;
    for (int i = 0; i < MAX_FILE_NUM; i++) dir.id[i] = -1;
    strcpy(dir.filenames[0], ".");
    dir.id[0] = root.id;
    root.addr[0] = (*lfs)->nextblock + 1; // inode pointer to directory content
    yrx_writeinode(*lfs, &root); // write inode to disk
    yrx_writedisk(*lfs, root.addr[0], &dir, sizeof(struct Directory));
    (*lfs)->nextblock++;
    return 0;
}

int fzw_readdisk(struct LFS*lfs,const int cache_num,const int block_num)
{
	log_msg2("[readdisk][cache_num=%d][block_num=%d]<%d>\n",cache_num,block_num,block_num*BLOCK_SIZE);
	int file;
	lfs->cache[cache_num].num=block_num;
	file=open(lfs->filename,O_RDWR);
	lseek(file,block_num*BLOCK_SIZE,SEEK_SET);
	read(file,lfs->cache[cache_num].block,BLOCK_SIZE);
	close(file);
	return 0;
}

int fzw_writedisk(struct LFS*lfs,const int cache_num)
{
	log_msg2("[writedisk][cache_num=%d][block_num=%d]<%d>\n",cache_num,lfs->cache[cache_num].num,lfs->cache[cache_num].num*BLOCK_SIZE);
	int file;
	file=open(lfs->filename,O_RDWR);
	lseek(file,lfs->cache[cache_num].num*BLOCK_SIZE,SEEK_SET);
	write(file,lfs->cache[cache_num].block,BLOCK_SIZE);
	close(file);
	return 0;
}

int fzw_read(struct LFS*lfs,const int block_num,void*block,const int size)
{
	log_msg2("[read][block_num=%d][size=%d]\n",block_num,size);
	int f1;
	for(f1=0;f1<CACHE_SIZE;f1++)
		if(lfs->cache[f1].num==block_num)
			break;
	if(f1==CACHE_SIZE)
	{
		f1=lfs->nt++;
		if(lfs->mv)
			fzw_writedisk(lfs,f1);
		fzw_readdisk(lfs,f1,block_num);
		if(lfs->nt==CACHE_SIZE)
		{
			lfs->nt=0;
			lfs->mv=1;
		}
	}
	memcpy(block,lfs->cache[f1].block,size);
	return 0;
}

int fzw_write(struct LFS*lfs,const int block_num,const void*block,const int size)
{
	log_msg2("[write][block_num=%d][size=%d]\n",block_num,size);
	int f1;
	for(f1=0;f1<CACHE_SIZE;f1++)
		if(lfs->cache[f1].num==block_num)
			break;
	if(f1==CACHE_SIZE)
	{
		f1=lfs->nt++;
		if(lfs->mv)
			fzw_writedisk(lfs,f1);
//		fzw_readdisk(lfs,f1,block_num);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if(lfs->nt==CACHE_SIZE)
		{
			lfs->nt=0;
			lfs->mv=1;
		}
	}
	lfs->cache[f1].num=block_num;
	memcpy(lfs->cache[f1].block,block,size);
//	fzw_writedisk(lfs,f1);
	return 0;
}

int fzw_sync(struct LFS*lfs)
{
	log_msg2("[sync]\n");
	int f1;
	if(lfs->mv)
		for(f1=0;f1<CACHE_SIZE;f1++)
			fzw_writedisk(lfs,f1);
	else
		for(f1=0;f1<lfs->nt;f1++)
			fzw_writedisk(lfs,f1);
	return 0;
}

int yrx_readdisk(struct LFS*lfs,int block_num,void*block,int size)
{
	if(block_num<0)
		return -1;
	pthread_mutex_lock(&lfs->dl);
	log_msg2("[readall][block_num=%d][size=%d]\n",block_num,size);
	int num;
	int f1;
	memset(block,0,size);
	num=(size-1)/BLOCK_SIZE;
	for(f1=0;f1<num;f1++)
		fzw_read(lfs,block_num+f1,block+f1*BLOCK_SIZE,BLOCK_SIZE);
	fzw_read(lfs,block_num+num,block+num*BLOCK_SIZE,size-num*BLOCK_SIZE);
	pthread_mutex_unlock(&lfs->dl);
	return 0;
}
int yrx_writedisk(struct LFS*lfs,int block_num,const void*block,int size)
{
	pthread_mutex_lock(&lfs->dl);
	log_msg2("[writeall][block_num=%d][size=%d]\n",block_num,size);
	int num;
	int f1;
	num=(size-1)/BLOCK_SIZE;
	for(f1=0;f1<num;f1++)
		fzw_write(lfs,block_num+f1,block+f1*BLOCK_SIZE,BLOCK_SIZE);
	fzw_write(lfs,block_num+num,block+num*BLOCK_SIZE,size-num*BLOCK_SIZE);
	pthread_mutex_unlock(&lfs->dl);
	return 0;
}

int yrx_writedir(struct LFS* lfs, struct Directory* dir) {
    int block_num = lfs->nextblock++;
    pthread_mutex_unlock(&(lfs->lock));
    yrx_writedisk(lfs, block_num, dir, sizeof(struct Directory));
    return 0;
}

int yrx_writeinode(struct LFS* lfs, struct INode* node) {
    pthread_mutex_lock(&(lfs->lock));
    lfs->superblock.inodemap[node->id] = lfs->nextblock++;
    pthread_mutex_unlock(&(lfs->lock));
    int block_num = lfs->superblock.inodemap[node->id];
    yrx_writedisk(lfs, block_num, node, sizeof(struct INode));
    return 0;
}
/*
int yrx_writedisk(struct LFS* lfs, int block_num, const void* block, int size) {
    FILE* file = fopen(lfs->filename, "r+");
    if (file == NULL) {
        perror("Error: ");
        return(-1);
    }
    fseek(file, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(block, size, 1, file); 
    fclose(file);
    return 0;
}
*/
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
/*
int yrx_readdisk(struct LFS* lfs, int block_num, void* block, int size) {
    if (block_num < 0) return 0;
    FILE* file = fopen(lfs->filename, "r");
    if (file == NULL) {
        perror("Error: ");
        return(-1);
    }
    fseek(file, block_num * BLOCK_SIZE, SEEK_SET);
    fread(block, size, 1, file);
    fclose(file);
    return 0;
}
*/
int yrx_readinode(struct LFS* lfs, int id, struct INode* node) {
    if (id < 0) return 0;
    int block_num = lfs->superblock.inodemap[id];
    if (lfs->superblock.inodemap[id] < 0) return 0; 
    yrx_readdisk(lfs, block_num, node, sizeof(struct INode));
    return 1;
}

int yrx_readdir(struct LFS* lfs, int block_num, struct Directory* dir) {
    yrx_readdisk(lfs, block_num, dir, sizeof(struct Directory));
    return 0;
}

int yrx_readinodefrompath(struct LFS* lfs, const char* path, struct INode* node, uid_t uid) {
    int found = 0;
    if (yrx_readinode(lfs, 0, node) == 0) return 0; 
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
                yrx_readdir(lfs, node->addr[0], &dir);
                for (int i = 0; i < MAX_FILE_NUM; ++i) {
                    if (strcmp(nextname, dir.filenames[i]) == 0) {
                        pthread_rwlock_rdlock(&(lfs->superblock.lockmap[dir.id[i]]));
                        found = yrx_readinode(lfs, dir.id[i], node);
                        pthread_rwlock_unlock(&(lfs->superblock.lockmap[dir.id[i]]));
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

int yrx_readfilefrominode(struct LFS* lfs, char* file, struct INode* node, size_t size, off_t offset) { 
    pthread_rwlock_rdlock(&(lfs->superblock.lockmap[node->id]));
    int index = 0;
    int true_size = min(size, node->size - offset);
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + true_size) / BLOCK_SIZE;
    int end = POINTER_PER_INODE - 1, addr = node->indir;
    struct Indirection* indir = malloc(sizeof(struct Indirection));
    char* buffer = malloc(BLOCK_SIZE * (true_size / BLOCK_SIZE + 2));
    if (end_block_num < POINTER_PER_INODE) {// no indirection
        for (int i = start_block_num; i <= end_block_num; ++i) {
            yrx_readdisk(lfs, node->addr[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
            index ++;
        }
        memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
    }
    else if (start_block_num < POINTER_PER_INODE) {
        for (int i = start_block_num; i < POINTER_PER_INODE; ++i) {
            yrx_readdisk(lfs, node->addr[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
            index++;
        }
        while (end < end_block_num) {
            if (addr < 0) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, addr, indir);
            addr = indir->indir;
            end += BLOCK_PER_INDIRECTION;
            if (end < end_block_num)
                for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                    yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
                    index ++;
                }
        }
        for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
            yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
            index ++;
        }
        memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
    }
    else {
        while (end < start_block_num) {
            if (addr < 0) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, addr, indir);
            end += BLOCK_PER_INDIRECTION;
            addr = indir->indir;
        }
        if (end_block_num <= end) {
            for (int i = (start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
                yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
                index ++;
            }
            memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
        }
        else {
            for (int i = (start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; i < BLOCK_PER_INDIRECTION; ++i) {
                yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
                index ++;
            }
            while (end < end_block_num) {
                if (addr < 0) yrx_init_indir(indir);
                else yrx_readindirectionblock(lfs, addr, indir);
                addr = indir->indir;
                end += BLOCK_PER_INDIRECTION;
                if (end < end_block_num)
                    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                        yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
                        index ++;
                    }
            }
            for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
                yrx_readdisk(lfs, indir->blocks[i], buffer + index * BLOCK_SIZE, BLOCK_SIZE);
                index ++;
            }
            memcpy(file, buffer + offset - start_block_num * BLOCK_SIZE, true_size);
        }
    }
    free(buffer);
    free(indir);
    pthread_rwlock_unlock(&(lfs->superblock.lockmap[node->id]));
    return 0;
}

int yrx_readdirfrominode(struct LFS* lfs, struct INode* node, struct Directory* dir) {
    return yrx_readdir(lfs, node->addr[0], dir); 
}

int yrx_readindirectionblock(struct LFS* lfs, int addr, struct Indirection* indir) {
    yrx_readdisk(lfs, addr, indir, sizeof(struct Indirection));
    return 0;
}

int yrx_renamefile(struct LFS* lfs, struct INode* fnode, const char* originname, const char* newname) {
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(originname, dir->filenames[i]) == 0) {
            strcpy(dir->filenames[i], newname);
            found = 1;
            break;
        }
    }
    free(dir);
    pthread_mutex_lock(&(lfs->lock));
    fnode->addr[0] = lfs->nextblock;
    yrx_writedir(lfs, dir);
    yrx_writeinode(lfs, fnode);
    return found;
}

int yrx_linkfile(struct LFS* lfs, struct INode* fnode, const char* filename, struct INode* node) {
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, fnode, dir);
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
    pthread_mutex_lock(&(lfs->lock));
    fnode->addr[0] = lfs->nextblock;
    yrx_writedir(lfs, dir);
    yrx_writeinode(lfs, node);
    yrx_writeinode(lfs, fnode);
    free(dir);
    return found;
}

int yrx_unlinkfile(struct LFS* lfs, struct INode* fnode, const char* filename) { // TODO: wtf?
    int found = 0;
    struct Directory* dir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, fnode, dir);
    for (int i = 0; i < MAX_FILE_NUM; ++i) {
        if (strcmp(dir->filenames[i], filename) == 0) {
            struct INode node;
            yrx_readinode(lfs, dir->id[i], &node);
            dir->id[i] = -1;
            found = 1;
            node.nlink --;
            if (node.nlink == 0) lfs->superblock.inodemap[node.id] = -1;
            else yrx_writeinode(lfs, &node); 
            break;
        }
    }
    pthread_mutex_lock(&(lfs->lock));
    fnode->addr[0] = lfs->nextblock;
    yrx_writedir(lfs, dir);
    yrx_writeinode(lfs, fnode);
    free(dir);
    return found;
}

int yrx_createinode(struct LFS* lfs, struct INode* node, mode_t mode, uid_t uid, gid_t gid) {
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
            pthread_rwlock_init(&(lfs->superblock.lockmap[i]), NULL);
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
int yrx_createdir(struct LFS* lfs, struct INode* fnode, const char* filename, mode_t mode, uid_t uid, gid_t gid) {  // TODO: relation with createfile
    struct Directory* fdir = malloc(sizeof(struct Directory));
    yrx_readdirfrominode(lfs, fnode, fdir); // read the parent dir
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
            yrx_createinode(lfs, node, mode, uid, gid);
            pthread_mutex_lock(&(lfs->lock));
            node->addr[0] = lfs->nextblock;
            node->nlink = 1;
            node->isdir = 1;
            dir->id[1] = node->id;
            yrx_writedir(lfs, dir);
            // update parent inode
            fnode->child_num ++;
            // update parent directory
            strcpy(fdir->filenames[i], filename);
            fdir->id[i] = node->id;
            yrx_writeinode(lfs, node);
            pthread_mutex_lock(&(lfs->lock));
            fnode->addr[0] = lfs->nextblock;
            yrx_writedir(lfs, fdir);
            yrx_writeinode(lfs, fnode);
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
int yrx_deletedir(struct LFS* lfs, struct INode* fnode, const char* filename) {
    struct Directory fdir;
    yrx_readdirfrominode(lfs, fnode, &fdir); // read the parent dir
    struct INode node;
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (strcmp(fdir.filenames[i], filename) == 0) {
            yrx_readinode(lfs, fdir.id[i], &node); // read the dir's inode
            if (node.isdir == 1) {
                if (node.child_num > 0) {
                    return 2; // still have child, cannot delete it
                }
                else {
                    lfs->superblock.inodemap[node.id] = -1; // delete the node
                    fdir.filenames[i][0] = '\0';  
                    fdir.id[i] = -1;
                    fnode->child_num --;
                    pthread_mutex_lock(&(lfs->lock));
                    fnode->addr[0] = lfs->nextblock;
                    yrx_writedir(lfs, &fdir);
                    yrx_writeinode(lfs, fnode);
                    return 1;
                }
            }
            else {
                return 0;
            }
        }
    }
    yrx_writeinode(lfs, fnode);
    return 0;
}

// write file:
// clear buffer
// write to block directly
// update the inode
int yrx_writefile(struct LFS* lfs, const char* file, struct INode* node, size_t size, off_t offset) { 
    // update the inode
    // [start, end]
    pthread_rwlock_wrlock(&(lfs->superblock.lockmap[node->id]));
    char* buffer = malloc(size + 2 * BLOCK_SIZE); // two extra blocks for begin and end
    int buffersize = 0;
    int start_block_num = offset / BLOCK_SIZE;
    int end_block_num = (offset + size) / BLOCK_SIZE;
    int maxend = POINTER_PER_INODE - 1;
    struct Indirection* indir = malloc(sizeof(struct Indirection));
    int address = node->indir;
    pthread_mutex_lock(&(lfs->lock));
    int startaddr = lfs->nextblock; // TODO: lock
    int realaddr = lfs->nextblock;
    lfs->nextblock += end_block_num - start_block_num + 1;
    pthread_mutex_unlock(&(lfs->lock));
    node->size = max(node->size, offset + size);
    node->blocks = (node->size - 1) / BLOCK_SIZE + 1;
    if (end_block_num < POINTER_PER_INODE) { // no need of indirection
        for (int i = start_block_num; i <= end_block_num; ++i) {
            if (node->addr[i] == -1) break;
            if (i == start_block_num || i == end_block_num) yrx_readdisk(lfs, node->addr[i], buffer + (i - start_block_num) * BLOCK_SIZE, BLOCK_SIZE);
        }
        for (int i = start_block_num; i <= end_block_num; ++i) node->addr[i] = startaddr + buffersize++;
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writedisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE);
        yrx_writeinode(lfs, node);
    }
    else if (start_block_num < POINTER_PER_INODE) {
        if (node->addr[start_block_num] != -1)
            yrx_readdisk(lfs, node->addr[start_block_num], buffer, BLOCK_SIZE);
        for (int i = start_block_num; i < POINTER_PER_INODE; ++i) {
            node->addr[i] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        pthread_mutex_lock(&(lfs->lock));
        int base = lfs->nextblock++;
        pthread_mutex_unlock(&(lfs->lock));
        node->indir = base;
        int nextbase = base;
        yrx_writeinode(lfs, node);
        while (maxend < end_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (end_block_num <= maxend) break;
            else {
                pthread_mutex_lock(&(lfs->lock));
                nextbase = lfs->nextblock++;
                pthread_mutex_unlock(&(lfs->lock));
                indir->indir = nextbase;
            }
            for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                indir->blocks[i] = realaddr;
                realaddr ++;
                buffersize ++;
            }
            yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
            base = nextbase;
        }
        yrx_readdisk(lfs, indir->blocks[(end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer + (end_block_num - start_block_num) * BLOCK_SIZE, BLOCK_SIZE);
        for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
            indir->blocks[i] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writedisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE);
    }
    else {
        pthread_mutex_lock(&(lfs->lock));
        int base = lfs->nextblock++;
        pthread_mutex_unlock(&(lfs->lock));
        node->indir = base;
        int nextbase = base;
        yrx_writeinode(lfs, node);
        while (maxend < start_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (maxend < end_block_num) {
                pthread_mutex_lock(&(lfs->lock));
                nextbase = lfs->nextblock++;
                pthread_mutex_unlock(&(lfs->lock));
                indir->indir = nextbase;
            }
            if (maxend < start_block_num) { 
                yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
                base = nextbase;
            }
        }
        yrx_readdisk(lfs, indir->blocks[(start_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer, BLOCK_SIZE);
        int minend = maxend;
        int same = 0;
        if (end_block_num < maxend){ // same indirection block
            minend = end_block_num;
            same = 1;
        }
        for (int i = start_block_num; i <= minend; ++i) {
            indir->blocks[(i-POINTER_PER_INODE)%BLOCK_PER_INDIRECTION] = realaddr;
            realaddr ++;
            buffersize ++;
        }
        yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
        // deal the first one 
        // Now indirection contains the start block
        base = nextbase;
        while (maxend < end_block_num) {
            if (address == -1) yrx_init_indir(indir);
            else yrx_readindirectionblock(lfs, address, indir);
            address = indir->indir;
            maxend += BLOCK_PER_INDIRECTION;
            if (end_block_num <= maxend) break;
            else {
                pthread_mutex_lock(&(lfs->lock));
                nextbase = lfs->nextblock++;
                pthread_mutex_unlock(&(lfs->lock));
                indir->indir = nextbase;
            }
            for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                indir->blocks[i] = realaddr ++;
                buffersize ++;
            }
            yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
            base = nextbase;
        }
        yrx_readdisk(lfs, indir->blocks[(end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION], buffer + end_block_num - start_block_num, BLOCK_SIZE);
        if (same == 0){
            for (int i = 0; i <= (end_block_num - POINTER_PER_INODE) % BLOCK_PER_INDIRECTION; ++i) {
                indir->blocks[i] = realaddr++;
                buffersize ++;
            }
            yrx_writedisk(lfs, base, indir, sizeof(struct Indirection));
        }
        memcpy(buffer+offset-start_block_num * BLOCK_SIZE, file, size);
        yrx_writedisk(lfs, startaddr, buffer, buffersize * BLOCK_SIZE);
    }
    free(buffer);
    free(indir);
    pthread_rwlock_unlock(&(lfs->superblock.lockmap[node->id]));
    return 0;
}

int yrx_begintransaction(struct LFS* lfs) {
    // TODO: clean
    if (lfs->nextblock > 1024 * 50 ){ // clean when half size filled up
        if (lfs->cleaning == 0){ 
            lfs->cleaning = 1;
            clean();
            lfs->cleaning = 0;
        }
    }
    //while(lfs->nextblock > 1024 * 90) { // stop writing when 90% size taken up
    //    sleep_ms(10);
    //}
    return 1;
}

int yrx_endtransaction(struct LFS* lfs) {
    FILE* file = fopen(lfs->filename, "r+");
    if (file == NULL) {
        perror("Error: ");
        return(-1);
    }
    lfs->superblock.nextblock = lfs->nextblock;
    fseek(file, 0, SEEK_SET);
    fwrite(&(lfs->superblock), sizeof(struct SuperBlock), 1, file); 
    fclose(file);
    return 1;
}

int clean() { // INode did not update file size
    if (lfs->nextblock < 1024 * 50) return 1;
    pthread_mutex_lock(&lfs->lock);
    char* buffer = malloc(lfs->nextblock * BLOCK_SIZE);
    struct INode node;
    int blockIndex = 0; 
    int blockOffset = 100;
    int inodemap[INODE_MAP_SIZE];
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (lfs->superblock.inodemap[i] > -1){
            // no more wr on this file
            pthread_rwlock_rdlock(&(lfs->superblock.lockmap[i]));
            yrx_readinode(lfs, i, &node);
        }
        else continue;
        if (node.isdir == 0 && node.addr[0] > -1) { // is non-empty file
            int base = node.blocks + blockIndex;
            yrx_readfilefrominode(lfs, buffer + blockIndex * BLOCK_SIZE, &node, node.size, 0);
            if (node.blocks <= POINTER_PER_INODE) for (int i = 0; i < node.blocks; ++i) node.addr[i] = blockOffset + blockIndex++;
            else {
                for (int i = 0; i < POINTER_PER_INODE; ++i) node.addr[i] = blockOffset + blockIndex++;
                int addr = node.indir;
                node.indir = base + blockOffset;
                while (addr > -1) {
                    struct Indirection indir;
                    yrx_readindirectionblock(lfs, addr, &indir);
                    addr = indir.indir;
                    for (int i = 0; i < BLOCK_PER_INDIRECTION; ++i) {
                        if (indir.blocks[i] > -1 ) indir.blocks[i] = blockOffset + blockIndex++;
                        else break;
                    }
                    if (addr > -1) indir.indir = ++base + blockOffset;
                    memcpy(buffer + (base - 1) * BLOCK_SIZE, &indir, sizeof(struct Indirection));
                }
                blockIndex = base;
            }
        }
        else if (node.addr[0] > -1) { // is directory
            struct Directory dir;
            yrx_readdirfrominode(lfs, &node, &dir);
            node.addr[0] = blockOffset + blockIndex;
            memcpy(buffer + blockIndex * BLOCK_SIZE, &dir, sizeof(struct Directory));
            blockIndex++;
        }
        inodemap[i] = blockIndex + blockOffset;
        memcpy(buffer + blockIndex * BLOCK_SIZE, &node, sizeof(struct INode));
        blockIndex++;
    }
    // no more rd/wr on whole file sys, wait for seg write onto disk
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (lfs->superblock.inodemap[i] > -1){
            pthread_rwlock_unlock(&(lfs->superblock.lockmap[i]));
            pthread_rwlock_wrlock(&(lfs->superblock.lockmap[i]));
            lfs->superblock.inodemap[i] = inodemap[i];
        }
    }
    lfs->nextblock = blockOffset; // TO DO: adjust later
    yrx_writedisk(lfs, lfs->nextblock, buffer, blockIndex * BLOCK_SIZE + sizeof(struct INode));
    lfs->nextblock += blockIndex;
    for (int i = 0; i < INODE_MAP_SIZE; ++i) {
        if (lfs->superblock.inodemap[i] > -1){
            pthread_rwlock_unlock(&(lfs->superblock.lockmap[i]));
        }
    }
    free(buffer);
    pthread_mutex_unlock(&lfs->lock);
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

int block_dump(struct LFS* lfs){
    FILE *file = fopen("/home/bys/block.txt", "a");
    fprintf(file, "\nBlock dump START!\n\n");
    SuperBlocktoString(file, &lfs->superblock);
    for (int i = 0; i < INODE_MAP_SIZE; i++){
        if (lfs->superblock.inodemap[i] != -1){
            struct INode* node = malloc(sizeof(struct INode));
            yrx_readinode(lfs, i, node);
            INodetoString(file, node);
            free(node);
        }
    }
    fprintf(file, "Block dump END!\n");
    fclose(file);
    return 0;
}
