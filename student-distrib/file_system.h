/* file_system.h - Defines functions necessary for file system
 * parsing and support
 */

#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"
#include "lib.h"
#include "syscall.h"

#define NAME_LEN 32			// Maximum length of file name
#define RESERVED_52 52		// Reserved 52B memory
#define RESERVED_24 24		// Reserved 24B memory
#define MAX_DENTRIES 63		// Maximum number of directory entries
#define MAX_DATA_BLOCKS 1023	// Maximum number of data blocks within an inode
#define DENTRY_SIZE 64		// Size of the directory entry struct (in Bytes)
#define BLOCK_SIZE 4096		// Size of each file system memory block
#define IN_USE 1			// Flag to signal 'in-use' of a file
#define FREE 0				// Flag to signal 'free' file descriptor
#define TYPE_RTC 0
#define TYPE_DIR 1
#define TYPE_FILE 2

/* Directory entry structure */
typedef struct dentry_t {
	uint8_t fname[NAME_LEN];
	uint32_t f_type;
	uint32_t inode_num;
	uint8_t reserved[RESERVED_24];
} dentry_t;

/* Boot Block */
typedef struct boot_block_t {
	uint32_t d_entries;
	uint32_t inodes;
	uint32_t d_blocks;
	uint8_t reserved[RESERVED_52];
	dentry_t dentry[MAX_DENTRIES];
} boot_block_t;

/* Inode structure */
typedef struct inode_t {
	uint32_t length;
	uint32_t data_blocks[MAX_DATA_BLOCKS];
} inode_t;

/* Pointer to FS boot block */
extern boot_block_t* fs_boot;

/* Helper functions to read files */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
uint32_t read_file_length(uint32_t inode);

/* File operations */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_open(int32_t fd);
int32_t file_close(int32_t fd);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

/* Directory operations */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_open(int32_t fd);
int32_t dir_close(int32_t fd);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

#endif
