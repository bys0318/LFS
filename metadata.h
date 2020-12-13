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
#define BLOCK_PER_INDIRECTION 255

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    int free_block_num;
    int free_inode_num;
};

struct LFS {
    char filename[MAX_FILENAME];
    struct SuperBlock superblock;
    int nextblock;
    int transaction;
    int buffersize;
    unsigned char buffer[MAX_BUFFER_SIZE];
};

struct Indirection {
    int blocks[BLOCK_PER_INDIRECTION];
    int indir;
};

extern struct LFS*lfs;

// initialization functions
int yrx_init_lfs(struct LFS** lfs);
int yrx_init_inode(struct INode* node, int isdir);
int yrx_init_indir(struct Indirection* indir);

// direct I/O, Basement of the filesystem
int yrx_readblockfromdisk(struct LFS* lfs, int block_num, void* block, int size, int time);
int yrx_writeblocktodisk(struct LFS* lfs, int block_num, const void* block, int size, int time);
int yrx_readblockfrombuffer(struct LFS* lfs, int block_num, void* block, int size, int time);
int yrx_writeblocktobuffer(struct LFS* lfs, const void* block, int size);
int yrx_writebuffertodisk(struct LFS* lfs);

// middle level I/O
int yrx_readinode(struct LFS* lfs, int tid, int id, struct INode* node);
int yrx_writeinode(struct LFS* lfs, int tid, struct INode* node);
int yrx_readdir(struct LFS* lfs, int tid, int blcok_num, struct Directory* dir);
int yrx_writedir(struct LFS* lfs, int tid, struct Directory* dir);

// high level I/O
int yrx_readinodefrompath(struct LFS* lfs, int tid, const char* path, struct INode* node, uid_t uid);// 0->not found, 1->succ, 2->no permission
int yrx_readdirectoryfrompath(struct LFS* lfs, int tid, const char* path, struct Directory* directory);
int yrx_readfilefrominode(struct LFS* lfs, int tid, char* file, struct INode* node, size_t size, off_t offset); 
int yrx_readdirfrominode(struct LFS* lfs, int tid, struct INode* node, struct Directory* dir);
int yrx_readindirectionblock(struct LFS* lfs, int tid, int addr, struct Indirection* indir);
int yrx_renamefile(struct LFS* lfs, int tid, struct INode* fnode, const char* originname, const char* newname);
int yrx_linkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename, struct INode* node);
int yrx_unlinkfile(struct LFS* lfs, int tid, struct INode* fnode, const char* filename);
int yrx_createinode(struct LFS* lfs, int tid, struct INode* node, mode_t mode, uid_t uid, gid_t gid);
int yrx_createdir(struct LFS* lfs, int tid, struct INode* fnode, const char* filename, mode_t mode, uid_t uid, gid_t gid);
int yrx_deletedir(struct LFS* lfs, int tid, struct INode* fnode, const char* filename);
int yrx_writefile(struct LFS* lfs, int tid, const char* file, struct INode* node, size_t size, off_t offset);

// transaction
int yrx_begintransaction(struct LFS* lfs);//transaction tid 0:ERROR
int yrx_endtransaction(struct LFS* lfs,int tid);//0:ERROR 1:Y

// permission check
int yrx_check_access(struct INode* node, uid_t uid);

// pretty print
int INodetoString(FILE *file, struct INode* node);
int SuperBlocktoString(FILE *file, struct SuperBlock* superblock);
int block_dump(struct LFS* lfs);
#endif
