/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR
#define MAX_PATH 4096
#define EXIT_FAILURE -1

//attribute libraries
#ifndef XATTR_USER_PREFIX
#define XATTR_USER_PREFIX "user."
#define XATTR_USER_PREFIX_LEN (sizeof (XATTR_USER_PREFIX) - 1)
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/xattr.h>
#include <linux/xattr.h>
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>

#endif
#include "aes-crypt.h"


typedef struct excParam{
	char* password;
	char* location;
}excParam;
#define Param ((struct excParam*) (fuse_get_context()->private_data))


static void getPath(char ppath[MAX_PATH], const char* path){
	strcpy(ppath,Param ->location);
	strncat(ppath, path, MAX_PATH);

}
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);

	res = lstat(ppath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);

	res = access(ppath, mask);
	if (res == -1)
		return -errno;

	return 0;
}
static void set_Attr(char* ppath){
	char* tmpstr = NULL;
	tmpstr = malloc(strlen("pa5-encfs.encrypted") + XATTR_USER_PREFIX_LEN + 1);
	if(!tmpstr){
	    perror("malloc of 'tmpstr' error");
	    exit(EXIT_FAILURE);
	}
	strcpy(tmpstr, XATTR_USER_PREFIX);
	strcat(tmpstr, "pa5-encfs.encrypted");
	/* Set attribute */
	if(setxattr(ppath, tmpstr, "true", strlen("true"), 0)){
	    perror("setxattr error");
	    fprintf(stderr, "path  = %s\n", ppath);
	    fprintf(stderr, "name  = %s\n", tmpstr);
	    fprintf(stderr, "value = %s\n", "true");
	    fprintf(stderr, "size  = %zd\n", strlen("true"));
	    exit(EXIT_FAILURE);
	}
	/* Cleanup */
	free(tmpstr);

}

static int get_Attr(const char* ppath){
	char* tmpstr = NULL;
	int valsize = -1;
	tmpstr = malloc(strlen("pa5-encfs.encrypted") + XATTR_USER_PREFIX_LEN + 1);
	if(!tmpstr){
	    perror("malloc of 'tmpstr' error");
	    exit(EXIT_FAILURE);
	}
	strcpy(tmpstr, XATTR_USER_PREFIX);
	strcat(tmpstr, "pa5-encfs.encrypted");
	/* Get attribute value size */
	valsize = getxattr(ppath, tmpstr, NULL, 0);

	return valsize;
}



static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);
	res = readlink(ppath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(ppath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(ppath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(ppath, mode);
	else
		res = mknod(ppath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);

	res = mkdir(ppath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);
	res = unlink(ppath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char ppath[MAX_PATH];
	getPath(ppath,path);

	res = rmdir(ppath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{

	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;

	res = chmod(ppath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;

	res = lchown(ppath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;

	res = truncate(ppath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(ppath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;

	res = open(ppath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
int fd;
int valsize;
int res;
char ppath[PATH_MAX];
getPath(ppath, path);

	valsize = get_Attr(ppath); // checks if the file is encrypted.
	if(valsize < 0){//if it is not, then it is not encrypted, we can read like usual.
		fd = open(ppath, O_RDONLY);
	if (fd == -1)
		return -errno;
	}else{ // if it is encrypted, we must dycrypt it.

  char filename[] = "/tmp/myfileXXXXXX"; // creates unique filename for the mkstemp.

  FILE* inFile = NULL;
  FILE* outFile= NULL;

  fd = mkstemp(filename); // creates and opens tmp file and gives us file descriptor.
  if(fd == -1){
    return -errno;
  }


  inFile = fopen(ppath, "rb"); // open your file
  if(!inFile){
    perror("infile fopen error");
    return EXIT_FAILURE;
  }
  outFile = fopen(filename, "wb+"); // open the tmp file
  if(!outFile){
    perror("outfile fopen error");
    return EXIT_FAILURE;
  }

  if(!do_crypt(inFile, outFile, 0, Param->password)){ // takes the encrypted text and dycrpts it and puts it in tmp file.
    fprintf(stderr, "do_crypt failed\n");
  }

  /* Cleanup */
  if(fclose(outFile)){
    perror("outFile fclose error\n");
  }
  if(fclose(inFile)){
    perror("inFile fclose error\n");
  }
	unlink(filename);
}

	(void) fi;

	res = pread(fd, buf, size, offset); // reads decrypted text.
	if (res == -1)
		res = -errno;

	close(fd);

	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	// Gets the path
	char ppath[PATH_MAX];
  getPath(ppath, path);

	int fd;
	int res;
  char filename[] = "/tmp/myfileXXXXXX";

  FILE* inFile = NULL;
  FILE* outFile= NULL;

	(void) fi;
  fd = mkstemp(filename);
  if(fd == -1){
    return -errno;
  }

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);

  /* Open Files */
  inFile = fopen(filename, "rb");
  if(!inFile){
    perror("infile fopen error");
    return EXIT_FAILURE;
  }
  outFile = fopen(ppath, "wb+");
  if(!outFile){
    perror("outfile fopen error");
    return EXIT_FAILURE;
  }

  if(!do_crypt(inFile, outFile, 1, Param->password)){ // encrypts the text and stores it in our text file.
    fprintf(stderr, "do_crypt failed\n");
  }

  /* Cleanup */
  if(fclose(outFile)){
    perror("outFile fclose error\n");
  }
  if(fclose(inFile)){
    perror("inFile fclose error\n");
  }
  unlink(filename); // releases the tmp file.
	set_Attr(ppath); // flags the written file encrypted.


	return res;
}
static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res;

	res = statvfs(ppath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
		char ppath[MAX_PATH];
		getPath(ppath,path);
    int res;
    res = creat(ppath, mode);
    if(res == -1)
	return -errno;

    close(res);
		set_Attr(ppath);

    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res = lsetxattr(ppath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res = lgetxattr(ppath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);
	int res = llistxattr(ppath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char ppath[MAX_PATH];
	getPath(ppath,path);


	int res = lremovexattr(ppath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	if(argc < 4){
		fprintf(stderr, " Not enough arguments\n");
		return -errno;
	}
	excParam* excParam_data;

	excParam_data = malloc(sizeof(excParam));
	excParam_data ->password = argv[1];
	excParam_data ->location = realpath(argv[argc-2],NULL);
	argv[1] = argv[3];
	argv[2] = NULL;
	argv[3] = NULL;
	argc = argc-2;
 fprintf(stdout, "%s\n",excParam_data ->location );
	return fuse_main(argc, argv, &xmp_oper, excParam_data);
}
