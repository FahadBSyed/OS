/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"


//treat this as our header file for our flat file and stuff. 
#ifndef SFS_HEADER
#define SFS_HEADER 1
#define USR_REGION 120

 unsigned char blocks[32768]; //represents which blocks are claimed by which files
							  //0 means free, 1 means kernel
							  
 struct stat filetable[128]; //represents possible flles							  
							  
 struct sfs_state *sfs_data;  //contains our diskfile and logfile references
 
 int file_id = 2;
 
 /*
	-16 MB disk.
	-128 file minimum. 
	-block size is 512. 
	-32,768 blocks. 
	- ideally we only need 100 blocks to do our disk metadata
	
	-need flat file or disk which we need to access and view as a hard drive. 
		
		- files
			- in units of blocks.
			- free space is considered a file. 
		
		- directories
			- represented as a file.
			- reference parent. (..)
			- reference to self. (.)
			- reference to all contained files and directories. 
			
		- inode: 
			- need a table of inodes
			- # hardlinks
			- group ID
			- owner ID
			- size
			- # blocks
			- array of block numbers in use. 
			
		- file descriptors: (current open files)
			- read/write/append/read+write.
			- owner ID
			- group ID
			- cursor position (same cursor for read and write?)
			
	- supposed to write in phase 1:
		- init
			- use testfsfile as the file representing our disk device 
			
		- getattr (done?)
		- readdir 
		
		
			
	
*/	
	



#endif


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "in sfs-init\n");
    log_msg("\nsfs_init()\n");
    
    log_conn(conn);
    log_fuse_context(fuse_get_context());
	
	int i = 0;
	for(i; i < 32768; i++){					//initialize our block map.
		if(i > USR_REGION) blocks[i] = 0;
		else blocks[i] = 1;
	}
	
	i = 0;
	char* buf = calloc(sizeof(struct stat), sizeof(struct stat));
	
	for(i; i < 128; i++){									//initailize our inode map. 
		memcpy(&filetable[i], buf, sizeof(struct stat));
	}
	free(buf);
	
	disk_open(sfs_data->diskfile); //open & create our disk block file. 
	
	i = 0;					//write to block map blocks.
	buf = malloc(512);
	for(i; i < 32768/512; i++){
		memcpy(buf, &blocks[i*512], 512);
		block_write(i, buf);
	}
	
	i = 0;					//write to inode map blocks. 
	for(i; i < sizeof(struct stat)/512; i++){
		memcpy(buf, (void*)((&filetable) + (i*512)), 512);
		block_write(i + 32768/512, buf);
	}
	free(buf);
	
	
    
	return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{	
    char fpath[PATH_MAX];
    int retstat = 0;
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
    
	printf( "GET ATTRIBUTE:\n" );
	printf( "\t%s\n", path );
	
	statbuf->st_gid = getgid(); // same group as mounting user
	statbuf->st_uid = getuid(); // same owner as mounting user
	statbuf->st_mtime = time(NULL); // just modified 
	statbuf->st_atime = time(NULL); // just accessed
	
	if ( strcmp(path, "/") == 0 )
	{
		statbuf->st_mode = 0755 | S_IFDIR;
		statbuf->st_nlink = 2; // Reason for 2 hardlinks: http://unix.stackexchange.com/a/101536
	}
	else
	{
		statbuf->st_mode = 0644 | S_IFREG;
		statbuf->st_size = 1024;
		statbuf->st_nlink = 1;
	}		
	
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    
	
	
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);

    
    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    
    return retstat;	
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

   
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    
    
    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);
    
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);
    
	DIR *dp;
    char fpath[PATH_MAX];
    strncat(fpath, path, 256);
	//strcpy(fpath, BB_DATA->rootdir);
	
    // since opendir returns a pointer, takes some custom handling of
    // return status.
    dp = opendir(fpath);
    log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
	retstat = -1;
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;	
	//every directory needs .. (prev directory) and . (current directory)
    filler( buf, ".", NULL, 0); // Current Directory
	filler( buf, "..", NULL, 0); // Parent Directory
	
	
	//read the root directory:
	
	//need to know:
		//how are directories represented
	
	
	if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
	{
	}
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
