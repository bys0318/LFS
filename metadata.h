#ifndef METADATA_H
#define METADATA_H

#define POINTER_PER_INODE 10
#define INODE_MAP_SIZE 1000
#define BLOCK_SIZE 1024
#define SUPERBLOCK_BLOCK 0
#define ROOT_INODE_BLOCK 1
#define MAX_FILENAME 20
#define MAX_FILE_NUM 20
#define MAX_BUFFER_SIZE 102400
#define BLOCK_PER_INDIRECTION 250
#define MODE_DIR_777 16895

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdlib.h>
#include <pthread.h>

struct INode {
    int id; // inode id
    off_t size; // file size
    int block_num; // where inode is
    mode_t mode; // type and permission
	nlink_t nlink; // hard link number
	uid_t uid; // user id
	gid_t gid; // group id
    unsigned long int blocks; // number of blocks the file use
    int addr[POINTER_PER_INODE]; // where the file is
    int indir;
    int isdir;
    int child_num;
    time_t atime;
	time_t mtime;
	time_t ctime; // create time, modify time, access time
};

struct Directory {
    char filenames[MAX_FILE_NUM][MAX_FILENAME];
    int id[MAX_FILE_NUM];
};

struct SuperBlock {
    int inodemap[INODE_MAP_SIZE];
    pthread_rwlock_t lockmap[INODE_MAP_SIZE];
    int nextblock;
};

#define CACHE_SIZE 2048
struct node
{
	int num;
	char block[BLOCK_SIZE];
};

struct LFS {
    char filename[MAX_FILENAME];
    struct SuperBlock superblock;
    int nextblock;
    pthread_mutex_t lock; // lock for nextblock
    int cleaning;
    pthread_mutex_t dl;
	int nt;
	int mv;
	struct node*cache;
};

struct Indirection {
    int blocks[BLOCK_PER_INDIRECTION];
    int indir;
};

extern struct LFS*lfs;

// initialization functions
int yrx_init_lfs(struct LFS** lfs, int reboot);
int yrx_init_inode(struct INode* node, int isdir);
int yrx_init_indir(struct Indirection* indir);

// direct I/O, Base of the filesystem
int yrx_readdisk(struct LFS* lfs, int block_num, void* block, int size);
int yrx_writedisk(struct LFS* lfs, int block_num, const void* block, int size);

// middle level I/O
int yrx_readinode(struct LFS* lfs, int id, struct INode* node);
int yrx_writeinode(struct LFS* lfs, struct INode* node);
int yrx_readdir(struct LFS* lfs, int block_num, struct Directory* dir);
int yrx_writedir(struct LFS* lfs, struct Directory* dir);

// high level I/O
int yrx_readinodefrompath(struct LFS* lfs, const char* path, struct INode* node, uid_t uid);// 0->not found, 1->succ, 2->no permission
int yrx_readfilefrominode(struct LFS* lfs, char* file, struct INode* node, size_t size, off_t offset); 
int yrx_readdirfrominode(struct LFS* lfs, struct INode* node, struct Directory* dir);
int yrx_readindirectionblock(struct LFS* lfs, int addr, struct Indirection* indir);
int yrx_renamefile(struct LFS* lfs, struct INode* fnode, const char* originname, const char* newname);
int yrx_linkfile(struct LFS* lfs, struct INode* fnode, const char* filename, struct INode* node);
int yrx_unlinkfile(struct LFS* lfs, struct INode* fnode, const char* filename);
int yrx_createinode(struct LFS* lfs, struct INode* node, mode_t mode, uid_t uid, gid_t gid);
int yrx_createdir(struct LFS* lfs, struct INode* fnode, const char* filename, mode_t mode, uid_t uid, gid_t gid);
int yrx_deletedir(struct LFS* lfs, struct INode* fnode, const char* filename);
int yrx_writefile(struct LFS* lfs, const char* file, struct INode* node, size_t size, off_t offset);

// clean up
int yrx_begintransaction(struct LFS* lfs);
int yrx_endtransaction(struct LFS* lfs);

// permission check
int yrx_check_access(struct INode* node, uid_t uid);
int checkusergroup(const uid_t uid, const gid_t gid);

// pretty print
int INodetoString(FILE *file, struct INode* node);
int SuperBlocktoString(FILE *file, struct SuperBlock* superblock);
int block_dump(struct LFS* lfs);

int clean();

void log_msg2(const char*const format,...);
int fzw_sync(struct LFS*lfs);

#endif
