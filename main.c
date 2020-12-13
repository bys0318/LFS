#define FUSE_USE_VERSION 31
#define PATH_MAX (1024)
#define LFS_DATA ((struct lfs_state *) fuse_get_context()->private_data)
#include"metadata.h"
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
struct lfs_state
{
	FILE*logfile;
	char*rootdir;
};
FILE*open_log()
{
	FILE*log;
	log=fopen("op.log", "w");
	setvbuf(log,NULL,_IOLBF,0);
	return log;
}
// Write message to the logfile
void log_msg(const char*const format,...)
{
	va_list ap;
	va_start(ap,format);
	vfprintf(LFS_DATA->logfile,format,ap);
	return;
}
// Get the full path (full_path = root + path)
static void get_full_path(char full_path[PATH_MAX], const char *path){
  strcpy(full_path, LFS_DATA->rootdir);
  strncat(full_path, path, PATH_MAX);
}
void*lfs_init(struct fuse_conn_info*conn,struct fuse_config*cfg)
{
	log_msg("INIT, <0x%08x>\n", conn);
	return LFS_DATA;
}
//OK!!!
int lfs_getattr(const char*path,struct stat*stbuf,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("GETATTR, <%s>, <0x%08x>, <0x%08x>\n", full_path, stbuf, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:log_msg("0\n");
			return -ENOENT;
		case 1:log_msg("1\n");
			stbuf->st_mode=node.mode;
			stbuf->st_nlink=node.nlink;
			stbuf->st_uid=node.uid;
			stbuf->st_gid=node.gid;
			stbuf->st_size=node.size;
			stbuf->st_blocks=node.blocks;
			stbuf->st_atime=node.atime;
			stbuf->st_mtime=node.mtime;
			stbuf->st_ctime=node.ctime;
			return 0;
		case 2:log_msg("2\n");
			return -EACCES;
		default:log_msg("???????????????????????\n");
			return -EACCES;
	}
}
//OK!!!
static int chmod_check(const struct INode*const node,const uid_t uid,const mode_t mode)
{
	return((uid==0)||(uid==node->uid));
}
//OK!!!
int lfs_chmod(const char*path,mode_t mode,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("CHMOD, <%s>, <%d>, <0x%08x>\n", full_path, mode, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	if(ret==1)
		if(chmod_check(&node,fuse_get_context()->uid,mode))
		{
			node.mode=mode;
			yrx_writeinode(lfs,tid,&node);
		}
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//TODO: checkusergroup!!!!!!
int checkusergroup(const uid_t uid,const gid_t gid)
{
	return fuse_get_context()->gid==gid;
}
//OK!!!
static int chown_check(const struct INode*const node,const uid_t uid,const uid_t nuid,const gid_t ngid)
{
	if(uid==0)
		return 1;
	if(uid!=node->uid)
		return 0;
	if((nuid!=uid)&&(nuid!=-1))
		return 0;
	if(checkusergroup(uid,ngid)||(ngid==-1))
		return 1;
	return 0;
}
//OK!!!
int lfs_chown(const char*path,uid_t uid,gid_t gid,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("CHOWN, <%s>, <%d>, <%d>, <0x%08x>\n", full_path, uid, gid, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	if(ret==1)
		if(chown_check(&node,fuse_get_context()->uid,uid,gid))
		{
			if(uid!=-1)
				node.uid=uid;
			if(gid!=-1)
				node.gid=gid;
			yrx_writeinode(lfs,tid,&node);
		}
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
static void pdn(const char*const p,char*const pd,char*const pn)//pd and pn should be memset to 0 before pdn!!!!!
{
	int len;
	int f1;
	len=strlen(p);
	for(f1=len-1;f1>=0;f1--)
		if(p[f1]=='/')
			break;
	if(f1==0)
		pd[0]='/';
	else
		memcpy(pd,p,f1);
	memcpy(pn,p+f1+1,len-f1-1);
	return;
}
//OK!!!
static int create_check(const struct INode*const node,const uid_t uid)
{
	if(uid==0)
		return 1;
	if(uid==node->uid)
		return (node->mode&S_IWUSR)&&(node->mode&S_IXUSR);
	if(checkusergroup(uid,node->gid))
		return (node->mode&S_IWGRP)&&(node->mode&S_IXGRP);
	else
		return (node->mode&S_IWOTH)&&(node->mode&S_IXOTH);
}
//尚未处理：文件已存在
int lfs_create(const char*path,mode_t mode,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("CREATE, <%s>, <%d>, <0x%08x>\n", full_path, mode, fi);
	int tid;
	int ret;
	struct INode node;
	struct INode fnode;
	char pd[PATH_MAX];
	char pn[PATH_MAX];
	memset(pd,0,PATH_MAX);
	memset(pn,0,PATH_MAX);
	pdn(path,pd,pn);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,pd,&fnode,fuse_get_context()->uid);
	if(ret==1)
		if(create_check(&fnode,fuse_get_context()->uid))
		{
			yrx_createinode(lfs,tid,&node,mode,fuse_get_context()->uid,fuse_get_context()->gid);
			yrx_linkfile(lfs,tid,&fnode,pn,&node);
		}
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//尚未处理：文件已存在
int lfs_rename(const char*from,const char*to,unsigned int flags)
{
	char full_path_from[PATH_MAX];
	get_full_path(full_path_from, from);
	char full_path_to[PATH_MAX];
	get_full_path(full_path_to, to);
	log_msg("RENAME, <%s>, <%s>, <%d>\n", full_path_from, full_path_to, flags);
	int tid;
	int ret;
	struct INode fnode1;
	struct INode fnode2;
	struct INode node;
	char pd1[PATH_MAX];
	char pn1[PATH_MAX];
	char pd2[PATH_MAX];
	char pn2[PATH_MAX];
	memset(pd1,0,PATH_MAX);
	memset(pn1,0,PATH_MAX);
	memset(pd2,0,PATH_MAX);
	memset(pn2,0,PATH_MAX);
	pdn(from,pd1,pn1);
	pdn(to,pd2,pn2);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,pd1,&fnode1,fuse_get_context()->uid);
	if(ret==1)
	{
		ret=yrx_readinodefrompath(lfs,tid,pd2,&fnode2,fuse_get_context()->uid);
		if(ret==1)
			if(create_check(&fnode1,fuse_get_context()->uid)&&create_check(&fnode2,fuse_get_context()->uid))
			{
				log_msg("----%d\n",yrx_readinodefrompath(lfs,tid,from,&node,fuse_get_context()->uid));
				yrx_linkfile(lfs,tid,&fnode2,pn2,&node);
				yrx_readinodefrompath(lfs,tid,pd1,&fnode1,fuse_get_context()->uid);
				yrx_unlinkfile(lfs,tid,&fnode1,pn1);log_msg("+++++++++++++++++++++++++++++++++++%s %s\n",pd1,pd2);
			}
			else
				ret=2;
	}
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
static int open_check(const struct INode*const node,const uid_t uid,const int flags)
{
	if(uid==0)
		return 1;
	if(uid==node->uid)
		switch(flags&O_ACCMODE)
		{
			case O_RDONLY:
				return node->mode&S_IRUSR;
			case O_WRONLY:
				return node->mode&S_IWUSR;
			case O_RDWR:
				return (node->mode&S_IRUSR)&&(node->mode&S_IWUSR);
			default:
				return 0;
		}
	if(checkusergroup(uid,node->gid))
		switch(flags&O_ACCMODE)
		{
			case O_RDONLY:
				return node->mode&S_IRGRP;
			case O_WRONLY:
				return node->mode&S_IWGRP;
			case O_RDWR:
				return (node->mode&S_IRGRP)&&(node->mode&S_IWGRP);
			default:
				return 0;
		}
	else
		switch(flags&O_ACCMODE)
		{
			case O_RDONLY:
				return node->mode&S_IROTH;
			case O_WRONLY:
				return node->mode&S_IWOTH;
			case O_RDWR:
				return (node->mode&S_IROTH)&&(node->mode&S_IWOTH);
			default:
				return 0;
		}
}
//OK!!!
int lfs_open(const char*path,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("OPEN, <%s>, <0x%08x>\n", full_path, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	yrx_endtransaction(lfs,tid);
	if((ret==1)&&(!open_check(&node,fuse_get_context()->uid,fi->flags)))
		ret=2;
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
static size_t min(const size_t a,const size_t b)
{
	if(a<b)
		return a;
	else
		return b;
}
//OK!!!
int lfs_read(const char*path,char*buf,size_t size,off_t offset,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("READ, <%s>, <0x%08x>, <%d>, <%lld>, <0x%08x>\n", full_path, buf, size, offset, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	if(offset<node.size)
	{
		ret=min(node.size-offset,size);
		yrx_readfilefrominode(lfs,tid,buf,&node,min(node.size-offset,size),offset);
	}
	else
		ret=0;
	yrx_endtransaction(lfs,tid);
	return ret;
}
//OK!!!
int lfs_write(const char*path,const char*buf,size_t size,off_t offset,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("WRITE, <%s>, <0x%08x>, <%d>, <%lld>, <0x%08x>\n", full_path, buf, size, offset, fi);
	int tid;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	yrx_writefile(lfs,tid,buf,&node,size,offset);
	yrx_endtransaction(lfs,tid);
	return size;
}
//OK!!!
static int link_check(const struct INode*const node,const uid_t uid)
{
	if((uid==0)||(uid==node->uid))
		return 1;
	if(checkusergroup(uid,node->gid))
		return (node->mode&S_IRGRP)&&(node->mode&S_IWGRP);
	else
		return (node->mode&S_IROTH)&&(node->mode&S_IWOTH);
}
//OK!!!
int lfs_link(const char*from,const char*to)
{
	char full_path_from[PATH_MAX];
	get_full_path(full_path_from, from);
	char full_path_to[PATH_MAX];
	get_full_path(full_path_to, to);
	log_msg("LINK, <%s>, <%s>\n", full_path_from, full_path_to);
	int tid;
	int ret;
	struct INode node;
	struct INode fnode;
	char pd[PATH_MAX];
	char pn[PATH_MAX];
	memset(pd,0,PATH_MAX);
	memset(pn,0,PATH_MAX);
	pdn(to,pd,pn);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,from,&node,fuse_get_context()->uid);
	if(ret==1)
	{
		ret=yrx_readinodefrompath(lfs,tid,pd,&fnode,fuse_get_context()->uid);
		if(ret==1)
			if(create_check(&fnode,fuse_get_context()->uid)&&link_check(&node,fuse_get_context()->uid))
				yrx_linkfile(lfs,tid,&fnode,pn,&node);
			else
				ret=2;
	}
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
int lfs_unlink(const char*path)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("UNLINK, <%s>\n", full_path);
	int tid;
	int ret;
	struct INode fnode;
	char pd[PATH_MAX];
	char pn[PATH_MAX];
	memset(pd,0,PATH_MAX);
	memset(pn,0,PATH_MAX);
	pdn(path,pd,pn);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,pd,&fnode,fuse_get_context()->uid);
	if(ret==1)
		if(create_check(&fnode,fuse_get_context()->uid))
			yrx_unlinkfile(lfs,tid,&fnode,pn);
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
int lfs_release(const char*path,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("RELEASE, <%s>, <0x%08x>\n", full_path, fi);
	return 0;
}
//OK!!!
static int opendir_check(const struct INode*const node,const uid_t uid)
{
	if(uid==0)
		return 1;
	if(uid==node->uid)
		return node->mode&S_IXUSR;
	if(checkusergroup(uid,node->gid))
		return node->mode&S_IXGRP;
	else
		return node->mode&S_IXOTH;
}
//OK!!!
int lfs_opendir(const char*path,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("OPENDIR, <%s>, <0x%08x>\n", full_path, fi);
	int tid;
	int ret;
	struct INode node;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	yrx_endtransaction(lfs,tid);
	if((ret==1)&&(!opendir_check(&node,fuse_get_context()->uid)))
		ret=2;
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
int lfs_mkdir(const char*path,mode_t mode)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("MKDIR, <%s>, <%d>\n", full_path, mode);
	int tid;
	int ret;
	struct INode fnode;
	char pd[PATH_MAX];
	char pn[PATH_MAX];
	memset(pd,0,PATH_MAX);
	memset(pn,0,PATH_MAX);
	pdn(path,pd,pn);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,pd,&fnode,fuse_get_context()->uid);
	if(ret==1)
		if(create_check(&fnode,fuse_get_context()->uid))
			yrx_createdir(lfs,tid,&fnode,pn,S_IFDIR|mode,fuse_get_context()->uid,fuse_get_context()->gid);
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
static int readdir_check(const struct INode*const node,const uid_t uid)
{
	if(uid==0)
		return 1;
	if(uid==node->uid)
		return node->mode&S_IRUSR;
	if(checkusergroup(uid,node->gid))
		return node->mode&S_IRGRP;
	else
		return node->mode&S_IROTH;
}
int lfs_readdir(const char*path,void*buf,fuse_fill_dir_t filler,off_t offset,struct fuse_file_info*fi,enum fuse_readdir_flags flags)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("READDIR, <%s>, <0x%08x>, <0x%08x>, <%lld>, <0x%08x>, \n", full_path, buf, filler, offset, fi);
	int tid;
	int ret;
	int f1;
	struct INode node;
	struct Directory dir;
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,path,&node,fuse_get_context()->uid);
	if(ret==1)
		if(readdir_check(&node,fuse_get_context()->uid))
		{
			yrx_readdirfrominode(lfs,tid,&node,&dir);
			for(f1=0;f1<MAX_FILE_NUM;f1++)
			{
				if(dir.id[f1]!=-1)
					log_msg("[name]%s\n",dir.filenames[f1]);
				if((dir.id[f1]!=-1)&&filler(buf,dir.filenames[f1],NULL,0,0))
					break;
			}
		}
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
//OK!!!
int lfs_releasedir(const char*path,struct fuse_file_info*fi)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("RELEASEDIR, <%s>, <0x%08x>\n", full_path, fi);
	return 0;
}
//OK!!!
int lfs_rmdir(const char*path)
{
	char full_path[PATH_MAX];
	get_full_path(full_path, path);
	log_msg("RMDIR, <%s>\n", full_path);
	int tid;
	int ret;
	struct INode fnode;
	char pd[PATH_MAX];
	char pn[PATH_MAX];
	memset(pd,0,PATH_MAX);
	memset(pn,0,PATH_MAX);
	pdn(path,pd,pn);
	tid=yrx_begintransaction(lfs);
	ret=yrx_readinodefrompath(lfs,tid,pd,&fnode,fuse_get_context()->uid);
	if(ret==1)
		if(create_check(&fnode,fuse_get_context()->uid))
			yrx_deletedir(lfs,tid,&fnode,pn);
		else
			ret=2;
	yrx_endtransaction(lfs,tid);
	switch(ret)
	{
		case 0:
			return -ENOENT;
		case 1:
			return 0;
		case 2:
			return -EACCES;
		default:
			return -EACCES;
	}
}
int lfs_utimens(const char*path,const struct timespec ts[2],struct fuse_file_info*fi)
{
	return 0;
}
struct fuse_operations lfs_op=
{
	.init=lfs_init,
	.getattr=lfs_getattr,
//	.getxattr=lfs_getxattr,
//	.access=lfs_access,
	.chmod=lfs_chmod,
	.chown=lfs_chown,
	.create=lfs_create,
	.rename=lfs_rename,
	.open=lfs_open,
	.read=lfs_read,
	.write=lfs_write,
//	.lseek=lfs_lseek,
	.link=lfs_link,
	.unlink=lfs_unlink,
//	.copy_file_range=lfs_copy_file_range,
	.release=lfs_release,
	.opendir=lfs_opendir,
	.mkdir=lfs_mkdir,
	.readdir=lfs_readdir,
	.releasedir=lfs_releasedir,
	.rmdir=lfs_rmdir,
//#ifdef HAVE_UTIMENSAT
	.utimens=lfs_utimens
//#endif
};
int main(int argc,char**argv)
{
	struct lfs_state*ourData;
	ourData=malloc(sizeof(struct lfs_state));
	ourData->rootdir=realpath(argv[argc-1],NULL);
	ourData->logfile=open_log();
	yrx_init_lfs(&lfs);
	return fuse_main(argc,argv,&lfs_op,ourData);
}
