/* filesystem.c - file system related variables and functions */

#include "filesystem.h"
#include "lib.h"

/* file system related constants */
#define BLOCK_SIZE 4096 // 4 kB
#define MAX_DENTRIES 63 // the maximum number of directory entries
// the maximum number of blocks in an inode
#define MAX_INODE_BLOCKS ((BLOCK_SIZE >> 2) - 1)
// the maximum size of a file
#define MAX_FILE_SIZE    (BLOCK_SIZE * MAX_INODE_BLOCKS)


/* data block structure */
typedef uint8_t data_block_t[BLOCK_SIZE];

/* inode structure */
typedef struct {
	uint32_t file_bytes;
	uint32_t data_blocks[MAX_INODE_BLOCKS];
} inode_t;

/* directory entry structure */
#define DENTRY_USEFUL_SIZE (MAX_NAME_LEN + 2 * sizeof(uint32_t))
#define DENTRY_TOTAL_SIZE  64
typedef struct {
	int8_t file_name[MAX_NAME_LEN];
	file_type_t file_type;
	uint32_t inode_num;
	uint8_t reserved[DENTRY_TOTAL_SIZE - DENTRY_USEFUL_SIZE];
} dentry_t;

/* Boot block structure - typedef'd in header */
#define STATISTICS_SIZE 64
struct boot_block_t {
	uint32_t num_dentries;
	uint32_t num_inodes;
	uint32_t num_data_blocks;
	uint8_t reserved[STATISTICS_SIZE - 3 * sizeof(uint32_t)];
	dentry_t dentries[MAX_DENTRIES];
};


/* Local variables */
static boot_block_t* boot_block;
static inode_t* inodes;
static data_block_t* data_blocks;


/* Local function macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define BYTES_TO_BLOCKS(x) (((x) + BLOCK_SIZE - 1) / BLOCK_SIZE)


/* Local functions */
static int32_t read_dentry_by_name(const int8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
static int32_t
read_data(uint32_t inode_num, uint32_t offset, uint8_t* buf, int32_t length);
static bool inode_is_valid(uint32_t inode_num);


/*
 * filesystem_init
 *   DESCRIPTION: Initializes the filesystem.
 *   INPUTS: boot_block_addr -- the address of the boot block.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets boot_block, inodes and data_blocks.
 */
void filesystem_init(boot_block_t* boot_block_addr) {
	boot_block = boot_block_addr;
	inodes = (inode_t*) (boot_block + 1);
	data_blocks = (data_block_t*) (inodes + boot_block->num_inodes);
}


/*
 * filesystem_open
 *   DESCRIPTION: Opens a file on the filesystem.
 *   INPUTS: filename -- the file name to read.
 *   OUTPUTS: file_desc -- a file descriptor to fill in.
 *   RETURN VALUE: the file type on success, FILE_TYPE_ERROR on failure
 *   SIDE EFFECTS: none.
 */
file_type_t filesystem_open(const int8_t* filename, file_desc_t* file_desc) {
	dentry_t dentry;
	
	if( filename[0] == '\0') //check null string case - unused dentry's will have a blank for their name, and there is no such thing as a file with no name
	{
		return FILE_TYPE_ERROR;
	}
	
	if (read_dentry_by_name(filename, &dentry) == -1) {
		return FILE_TYPE_ERROR;
	}

	file_desc->file_pos = 0;
	// will get ignored for RTC and directories
	file_desc->inode_num = dentry.inode_num;

	return dentry.file_type;
}


/*
 * file_read
 *   DESCRIPTION: Reads from a file.
 *   INPUTS: file_desc -- the file descriptor to read from.
 *           nbytes -- the number of bytes to read.
 *   OUTPUTS: buf -- the read bytes.
 *   RETURN VALUE: The number of bytes read, or -1 on failure.
 *   SIDE EFFECTS: none.
 */
int32_t file_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	int32_t bytes_read = 
		read_data(file_desc->inode_num, file_desc->file_pos, buf, nbytes);
	if (bytes_read != -1) {
		file_desc->file_pos += bytes_read;
	}
	return bytes_read;
}


/*
 * dir_read
 *   DESCRIPTION: Reads from a directory.
 *   INPUTS: file_desc -- the file descriptor to read from.
 *           nbytes -- the number of bytes to read.
 *   OUTPUTS: buf -- the read bytes.
 *   RETURN VALUE: The number of bytes read, or -1 on failure.
 *   SIDE EFFECTS: none.
 */
int32_t dir_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	if (nbytes <= 0) {
		// returns -1 for nbytes < 0 and 0 for nbytes == 0
		return -(nbytes < 0);
	}

	dentry_t dentry;
	if (read_dentry_by_index(file_desc->file_pos, &dentry) == -1) {
		// reached end of directory
		return 0;
	}

	++file_desc->file_pos;
	int32_t copy_bytes = MIN(nbytes, MAX_NAME_LEN);
	memcpy(buf, dentry.file_name, copy_bytes);
	return copy_bytes;
}


/*
 * read_dentry_by_name
 *   DESCRIPTION: Reads a directory entry by name.
 *   INPUTS: fname -- the file name of the entry to read.
 *   OUTPUTS: dentry -- the read directory entry.
 *   RETURN VALUE: 0 on success, -1 for nonexistent file.
 *   SIDE EFFECTS: none.
 */
static int32_t read_dentry_by_name(const int8_t* fname, dentry_t* dentry) {
	// since I'm using strncmp, I'll get a false positive if the filename is
	// MAX_NAME_LEN bytes and a prefix of fname, so I'm explicitly checking
	// for names which are too long
	if (strlen(fname) > MAX_NAME_LEN) {
		return -1;
	}

	// do a linear search through the directory entries
	int i;
	for (i = 0; i < MAX_DENTRIES; ++i) {
		dentry_t* cur_entry = &boot_block->dentries[i];
		if (strncmp(fname, cur_entry->file_name, MAX_NAME_LEN) == 0) {
			memcpy(dentry, cur_entry, DENTRY_USEFUL_SIZE);
			return 0;
		}
	}

	// the search failed
	return -1;
}


/*
 * read_dentry_by_index
 *   DESCRIPTION: Reads a directory entry by index.
 *   INPUTS: index -- the index of the entry to read.
 *   OUTPUTS: dentry -- the read directory entry.
 *   RETURN VALUE: 0 on success, -1 for nonexistent index.
 *   SIDE EFFECTS: none.
 */
static int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
	// check that index lies within the boundary
	if (index >= 0 && index < boot_block->num_dentries) {
		dentry_t* curr_entry = &boot_block->dentries[index];
		memcpy(dentry, curr_entry, DENTRY_USEFUL_SIZE);
		return 0;
	}
	return -1;
}


/*
 * read_data
 *   DESCRIPTION: Reads data from an inode.
 *   INPUTS: inode_num -- the inode number to read from.
 *           offset -- the offset within the inode to start reading from.
 *           length -- the number of bytes to read.
 *   OUTPUTS: buf -- the read data.
 *   RETURN VALUE: The number of bytes read on success, -1 on error.
 *   SIDE EFFECTS: none.
 */
static int32_t
read_data(uint32_t inode_num, uint32_t offset, uint8_t* buf, int32_t length) {
	// check if we are truly at inode and length is sensible
	if (length < 0 || !inode_is_valid(inode_num)) {
		return -1;
	}

	inode_t* inode = &inodes[inode_num];
	// trivial reads
	if (length == 0 || offset >= inode->file_bytes) {
		return 0;
	}

	// copy data over
	int32_t read_bytes = MIN(length, inode->file_bytes - offset);
	int32_t bytes_remaining = read_bytes;
	uint32_t inode_block = offset / BLOCK_SIZE;
	uint32_t block_offset = offset % BLOCK_SIZE;
	while (bytes_remaining > 0) {
		int32_t bytes_to_copy = MIN(bytes_remaining, BLOCK_SIZE - block_offset);
		uint32_t block_num = inode->data_blocks[inode_block];
		memcpy(buf, (uint8_t*) &data_blocks[block_num] + block_offset, bytes_to_copy);
		buf += bytes_to_copy;
		bytes_remaining -= bytes_to_copy;
		++inode_block;
		block_offset = 0;
	}
	return read_bytes;
}


/*
 * inode_is_valid
 *   DESCRIPTION: Checks if an inode is valid.
 *   INPUTS: inode_num -- the inode number to check.
 *   OUTPUTS: none.
 *   RETURN VALUE: true if inode is valid (inode_num in range and data sensible)
 *                 false otherwise
 *   SIDE EFFECTS: none.
 */
static bool inode_is_valid(uint32_t inode_num) {
	if (inode_num > boot_block->num_inodes) {
		// bad inode number
		return false;
	}

	inode_t* inode = &inodes[inode_num];
	if (inode->file_bytes > MAX_FILE_SIZE) {
		// nonsensical size
		return false;
	}

	// loop through all data block numbers
	uint32_t i;
	uint32_t num_blocks = BYTES_TO_BLOCKS(inode->file_bytes);
	for (i = 0; i < num_blocks; ++i) {
		if (inode->data_blocks[i] > boot_block->num_data_blocks) {
			// nonsensical data block
			return false;
		}
	}

	// all tests passed so we're good
	return true;
}
