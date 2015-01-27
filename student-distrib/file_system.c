/* file_system.c -
*
*/

#include "file_system.h"

boot_block_t* fs_boot; // Globally shared pointer to file system

/*
* int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
*	Inputs: const uint8_t* fname = file name
*			dentry_t* dentry = directory entry
*	Return Value: 0 upon successful read, -1 on failure
*	Function: Fill in dentry block with file name, type and inode based on name
*/
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
	int i, length;
	
	/* Get file name length */
	length = strlen((int8_t*)fname);
	
	/*Check for NULL filename */
	if (strlen((int8_t*)fname) == 0)
		return -1;
		
	/* Check for long filename */
	if (length >= NAME_LEN)
		length = NAME_LEN - 1;
	
	/* Transverse directory entries to match file name */
	for (i = 0; i < fs_boot->d_entries; i++) {
		if (strncmp((int8_t*)fname, (int8_t*)fs_boot->dentry[i].fname, length) == 0) {
			/* Copy directory entry struct to dentry */
			memcpy(dentry, &fs_boot->dentry[i], DENTRY_SIZE);
			return 0;
		}
	}
	
	/* Non-existent file */
	return -1;
}

/*
* int32_t read_dentry_by_index(const uint8_t* index, dentry_t* dentry)
*	Inputs: const uint8_t* index = inode number
*			dentry_t* dentry = directory entry
*	Return Value: 0 upon successful read, -1 on failure
*	Function: Fill in dentry block with file name, type and inode based on index
*/
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {	
	/* Check for invalid inode number */
	if (index > fs_boot->d_entries)
		return -1;
		
	/* Otherwise, copy directory entry struct to dentry */
	memcpy(dentry, &fs_boot->dentry[index], DENTRY_SIZE);
	return 0;
}

/*
* int32_t read_data(uint32_t inode, uint32_t offset, uint32_t* buf, uint32_t length)
*	Inputs: uint32_t inode = inode number
*			uint32_t offset = position within the file
*			uint32_t* buf = buffer that holds the data read
*			uint32_t length = number of bytes to read from file
*	Return Value: -1 for bad data block number, 0 if end of file reached or
*					N number of bytes read into buffer
*	Function: Copy data from data blocks to buf
*/
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
	/* Check for invalid inode */
	if (inode > fs_boot->inodes)
		return -1;
		
	/* Check for non-existent buff */
	if (buf == NULL)
		return -1;
	
	/* Find address to the inode block */
	uint32_t* inode_addr = (uint32_t*)((uint32_t)fs_boot + ((inode + 1) * BLOCK_SIZE));
	uint32_t f_length = *inode_addr;
	
	/* Check if referencing out of bound position */
	if (offset > f_length)
		return -1;
		
	uint32_t* data_base_addr = (uint32_t*)((uint32_t)fs_boot + BLOCK_SIZE + (fs_boot->inodes * BLOCK_SIZE));
	
	uint32_t data_block_index = *(inode_addr + (offset / BLOCK_SIZE) + 1);
	
	uint32_t* data_addr = (uint32_t*)((uint32_t)data_base_addr + (data_block_index * BLOCK_SIZE));
	
	uint32_t bytes_read = 0;
	char* tracker;
	
	while (bytes_read < length) {
		/* Check for eof */
		if (offset + bytes_read > f_length)
			return 0;
			
		/* Check for end of block */
		if (((offset + bytes_read) % BLOCK_SIZE) == 0) {
			data_block_index = *(inode_addr + (((offset + bytes_read) / BLOCK_SIZE) + 1));
			data_addr = (uint32_t*)((uint32_t)data_base_addr + (data_block_index * BLOCK_SIZE));
		}
		
		tracker = (char*)data_addr + ((offset + bytes_read) % BLOCK_SIZE);
		buf[bytes_read] = *tracker; 
		bytes_read++;
	}
	return bytes_read;
}

/*
* uint32_t read_file_length(uint32_t inode)
*	Inputs: uint32_t inode = inode number
*	Return Value: file length
*	Function: Obtain the file length of given inode number
*/
uint32_t read_file_length(uint32_t inode) {
	/* Find address to the inode block */
	uint32_t* inode_addr = (uint32_t*)((uint32_t)fs_boot + ((inode + 1) * BLOCK_SIZE));
	uint32_t f_length = *inode_addr;
	return f_length;
}

/*
* int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
*	Inputs: uint8_t* buf = buffer that holds the data read
*			uint32_t length = number of bytes to read from file
*			file_desc_t* file_desc = pointer to file descriptor block
*	Return Value: -1 for bad data block number, 0 if end of file reached or
*					N number of bytes read into buffer
*	Function: Read and copy info from file to buf
*/
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
/*	dentry_t temp;
	
	read_dentry_by_name(fname, &temp);
*/	//Remember to delete commented (if this setup works
	int32_t bytes_read = read_data(current_pcb->file_array[fd].inode_num, current_pcb->file_array[fd].file_pos, buf, nbytes);
	current_pcb->file_array[fd].file_pos += bytes_read;


	return bytes_read;
}

/*
* int32_t dir_read(int32_t fd, void* buf, int32_t nbytes)
*	Inputs: uint8_t* buf = buffer that holds the data read
*			uint32_t length = number of bytes to read from file
*			file_desc_t* file_desc = pointer to file descriptor block
*	Return Value: -1 for bad data block number, 0 if end of file reached or
*					N number of bytes read into buffer
*	Function: Fill in buf with file names
*/
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {
	dentry_t temp;
	
	/* Check for empty entries */
	if (fs_boot->d_entries == 0)
		return 0;
	
	/* Check for end of directory entries */
	if (current_pcb->file_array[fd].file_pos >= fs_boot->d_entries)
		return 0;
	
	if (!read_dentry_by_index(current_pcb->file_array[fd].file_pos, &temp)) {
		strncpy((int8_t*)buf, (int8_t*)temp.fname, nbytes);
		current_pcb->file_array[fd].file_pos++;
		return nbytes;
	}
		
	return -1;
}

/*
* int32_t file_open(int32_t fd)
*	Inputs: file_desc_t* file = pointer to file struct
*			int32_t inode_num = inode number of file
*	Return Value: 0 upon successful initialization
*	Function: Opens a file
*/
int32_t file_open(int32_t fd) {
	current_pcb->file_array[fd].file_pos = 0; // Initializes position to beginning of file
	current_pcb->file_array[fd].flags = IN_USE; // Set file to in use

	return 0;
}

/*
* int32_t dir_open(int32_t fd)
*	Inputs: int32_t fd = 
*	Return Value: 0 upon successful initialization
*	Function: Opens a directory
*/
int32_t dir_open(int32_t fd) {
	current_pcb->file_array[fd].file_pos = 0; // Initializes to beginning of directory entries
	current_pcb->file_array[fd].inode_num = NULL; // Set to NULL for directory
	current_pcb->file_array[fd].flags = IN_USE; // Set directory to in use
	
	return 0;
}

/*
* int32_t file_close(int32_t fd)
*	Inputs: none
*	Return Value: return 0
*	Function: Closes a file
*/
int32_t file_close(int32_t fd) {
	return 0;
}

/*
* int32_t dir_close(int32_t fd)
*	Inputs: none
*	Return Value: return 0
*	Function: Closes a directory
*/
int32_t dir_close(int32_t fd) {
	return 0;
}

/*
* int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
*	Inputs: none
*	Return Value: return -1
*	Function: Writing to file not supported
*/
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
	return -1;
}

/*
* int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
*	Inputs: none
*	Return Value: return -1
*	Function: Writing to directory not supported
*/
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes) {
	return -1;
}

