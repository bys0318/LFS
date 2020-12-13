// gcc -Wall lfs.c `pkg-config fuse3 --cflags --libs` -o lfs
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>

struct lfs_state {
    FILE *logfile;
    char *rootdir;
};
#define LFS_DATA ((struct lfs_state *) fuse_get_context()->private_data)
#define PATH_MAX 1024

FILE *open_log(){
  FILE *log;
  log = fopen("op.log", "w");
  setvbuf(log, NULL, _IOLBF, 0);
  return log;
}

// Write message to the logfile
void log_msg(const char *format, ...){
    va_list ap;
    va_start(ap, format);
    vfprintf(LFS_DATA->logfile, format, ap);
}

// Get the full path (full_path = root + path)
static void get_full_path(char full_path[PATH_MAX], const char *path){
  strcpy(full_path, LFS_DATA->rootdir);
  strncat(full_path, path, PATH_MAX);
}

// Initialize filesystem
void *lfs_init(struct fuse_conn_info *conn)
{
    log_msg("INIT, <0x%08x>\n", conn);
    
    return LFS_DATA;
}

// Check file access permissions, this will be called for the access() system call
int lfs_access(const char *path, int mask){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("ACCESS, <%s>, <%d>\n", full_path, mask);
  return 0;
}

// Get file attributes
int lfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
  (void) fi;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("GETATTR, <%s>, <0x%08x>, <0x%08x>\n", full_path, stbuf, fi);
  memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
  else{
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = 0;
  }
  return 0;
}

// Get extended attributes
int lfs_getxattr(const char *path, const char *name, char *value, size_t size){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("GETXATTR, <%s>, <0x%08x>, <0x%08x>, <%d>\n", full_path, name, value, size);
  return 0;
}

// Change the permission bits of a file
int lfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi){
  (void) fi;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("CHMOD, <%s>, <%d>, <0x%08x>\n", full_path, mode, fi);
  return 0;
}

// Change the owner and group of a file
int lfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi){
  (void) fi;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("CHOWN, <%s>, <%d>, <%d>, <0x%08x>\n", full_path, uid, gid, fi);
  return 0;
}

// Create and open a file
int lfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("CREATE, <%s>, <%d>, <0x%08x>\n", full_path, mode, fi);
  return 0;
}

// Rename a file
int lfs_rename(const char *from, const char *to, unsigned int flags){
  char full_path_from[PATH_MAX];
  get_full_path(full_path_from, from);
  char full_path_to[PATH_MAX];
  get_full_path(full_path_to, to);
  log_msg("RENAME, <%s>, <%s>, <%d>\n", full_path_from, full_path_to, flags);
  return 0;
}

// Open a file
int lfs_open(const char *path, struct fuse_file_info *fi){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("OPEN, <%s>, <0x%08x>\n", full_path, fi);
  return 0;
}

/*
* Read data from an open file, read should return exactly the number of bytes requested except on EOF or error, 
* otherwise the rest of the data will be substituted with zeroes
*/
int lfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
  (void) fi;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("READ, <%s>, <0x%08x>, <%d>, <%lld>, <0x%08x>\n", full_path, buf, size, offset, fi);
  return 0;
}

// Write data to an open file, write should return exactly the number of bytes requested except on error
int lfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
  (void) fi;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("WRITE, <%s>, <0x%08x>, <%d>, <%lld>, <0x%08x>\n", full_path, buf, size, offset, fi);
  return 0;
}

// Find next data or hole after the specified offset
int lfs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("LSEEK, <%s>, <%lld>, <%d>, <0x%08x>\n", full_path, off, whence, fi);
  return 0;
}

// Create a hard link to a file
int lfs_link(const char *from, const char *to){
  char full_path_from[PATH_MAX];
  get_full_path(full_path_from, from);
  char full_path_to[PATH_MAX];
  get_full_path(full_path_to, to);
  log_msg("LINK, <%s>, <%s>\n", full_path_from, full_path_to);
  return 0;
}

// Remove a file
int lfs_unlink(const char *path){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("UNLINK, <%s>\n", full_path);
  return 0;
}

/*
* Copy a range of data from one file to another: Performs an optimized copy between two file descriptors 
* without the additional cost of transferring data through the FUSE kernel module to user space (glibc) 
* and then back into the FUSE filesystem again
*/
int lfs_copy_file_range(const char *path_in, struct fuse_file_info *fi_in, off_t offset_in, 
      const char *path_out, struct fuse_file_info *fi_out, off_t offset_out, size_t len, int flags){
  char full_path_in[PATH_MAX];
  get_full_path(full_path_in, path_in);
  char full_path_out[PATH_MAX];
  get_full_path(full_path_out, path_out);
  log_msg("COPY_FILE_RANGE, <%s>, <0x%08x>, <%d>, <%s>, <0x%08x>, <%d>, <%d>, <%d>\n", 
          full_path_in, fi_in, offset_in, full_path_out, fi_out, offset_out, len, flags);
	return 0;
}

/*
* Release an open file, release is called when there are no more references to an open file: 
* all file descriptors are closed and all memory mappings are unmapped
*/
int lfs_release(const char *path, struct fuse_file_info *fi){
	char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("RELEASE, <%s>, <0x%08x>\n", full_path, fi);
	return 0;
}

// Open directory, this method should check if opendir is permitted for this directory
int lfs_opendir(const char *path, struct fuse_file_info *fi){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("OPENDIR, <%s>, <0x%08x>\n", full_path, fi);
  return 0;
}

// Create a directory
int lfs_mkdir(const char *path, mode_t mode){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("MKDIR, <%s>, <%d>\n", full_path, mode);
  return 0;
}

// Read directory
int lfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, 
        struct fuse_file_info *fi, enum fuse_readdir_flags flags){
  (void) offset;
  (void) fi;
  (void) flags;
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("READDIR, <%s>, <0x%08x>, <0x%08x>, <%lld>, <0x%08x>, \n", full_path, buf, filler, offset, fi);
  filler(buf, ".", NULL, 0, 0);
  filler(buf, "..", NULL, 0, 0);
  return 0;
}

// Release directory
int lfs_releasedir(const char *path, struct fuse_file_info *fi){
	char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("RELEASEDIR, <%s>, <0x%08x>\n", full_path, fi);
	return 0;
}

// Remove a directory
int lfs_rmdir(const char *path){
  char full_path[PATH_MAX];
  get_full_path(full_path, path);
  log_msg("RMDIR, <%s>\n", full_path);
  return 0;
}

static struct fuse_operations lfs_op = {
  .init = lfs_init,
  .getattr = lfs_getattr,
  .getxattr = lfs_getxattr,
  .access = lfs_access,
  .chmod = lfs_chmod,
  .chown = lfs_chown,
  .create = lfs_create,
  .rename = lfs_rename,
  .open = lfs_open,
  .read = lfs_read,
  .write = lfs_write,
  .lseek = lfs_lseek,
  .link = lfs_link,
  .unlink = lfs_unlink,
  .copy_file_range = lfs_copy_file_range,
  .release = lfs_release,
  .opendir = lfs_opendir,
  .mkdir = lfs_mkdir,
  .readdir = lfs_readdir,
  .releasedir = lfs_releasedir,
  .rmdir = lfs_rmdir,
};

int main(int argc, char *argv[]){
  struct lfs_state *ourData;
  ourData = malloc(sizeof(struct lfs_state));
  fprintf(stderr, "%d\n", argc);
  fprintf(stderr, "%s\n", argv[0]);
  fprintf(stderr, "%s\n", argv[1]);
  ourData->rootdir = realpath(argv[argc-1], NULL);
  ourData->logfile = open_log();

  fprintf(stderr, "ready to call fuse_main\n");
  int stat = fuse_main(argc, argv, &lfs_op, ourData);
  fprintf(stderr, "exiting fuse_main\n");
  return stat;
}
