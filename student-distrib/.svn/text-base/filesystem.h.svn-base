/* filesystem.h - file system related structures and prototypes */

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"
#include "file_descriptor.h"

/* externally useful file system related constants */
#define MAX_NAME_LEN 32 // the maximum length of a filename

/* Boot block structure - internal */
typedef struct boot_block_t boot_block_t;

/* Initializes the filesystem */
extern void filesystem_init(boot_block_t* boot_block_addr);

/* Opens a file on the filesystem */
extern file_type_t
filesystem_open(const int8_t* filename, file_desc_t* file_desc);

/* Reads a file */
extern int32_t file_read(file_desc_t* file_desc, void* buf, int32_t nbytes);

/* Reads a directory */
extern int32_t dir_read(file_desc_t* file_desc, void* buf, int32_t nbytes);

#endif
