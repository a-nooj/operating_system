/* file_descriptor.h - structures and prototypes for file system abstractions */

#ifndef _FILE_DESCRIPTOR_H
#define _FILE_DESCRIPTOR_H

#include "types.h"

/* File types enumeration */
typedef enum {
	FILE_TYPE_ERROR = -1,
	FILE_TYPE_RTC,
	FILE_TYPE_DIR,
	FILE_TYPE_FILE,
	FILE_TYPE_STDIN,
	FILE_TYPE_STDOUT,
	FILE_TYPE_ADLIB,
	NUM_FILE_TYPES
} file_type_t;

// forward declaration, needed because of circular reference
typedef struct file_ops_t file_ops_t;

/* File descriptor structure */
typedef struct {
	file_ops_t* file_ops;
	uint32_t inode_num;
	uint32_t file_pos;
	uint32_t flags;
} file_desc_t;

/* Flags constants */
// what others do we need?
#define DESC_IN_USE 1

/* File operations structure */
struct file_ops_t {
	int32_t (*open)(void);
	int32_t (*close)(void);
	int32_t (*read)(file_desc_t* file_desc, void* buf, int32_t nbytes);
	int32_t (*write)(file_desc_t* file_desc, const void* buf, int32_t nbytes);
};

/* Predefined file operations structures */
extern file_ops_t file_ops[NUM_FILE_TYPES];

/* File descriptor table */
#define MAX_OPEN_FILES 8

// will need to be updated on a context switch
extern file_desc_t* file_desc_table;


/* File descriptor functions */
// string arguments changed to int8_t* to be consistent with lib.h
extern int32_t open(const int8_t* filename);
extern int32_t close(int32_t fd);
extern int32_t read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes);

#endif
