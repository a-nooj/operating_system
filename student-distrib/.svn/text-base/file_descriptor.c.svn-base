/* file_descriptor.h - structures and functions for file system abstractions */

#include "file_descriptor.h"
#include "filesystem.h"
#include "terminal.h"
#include "rtc.h"
#include "soundblaster.h"

/* Predefined file operations functions and structures */

// null and invalid functions to avoid cluttering up other files
static int32_t null_open_close(void) {
	return 0;
}

static int32_t invalid_open_close(void) {
	return -1;
}

static int32_t
invalid_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	return -1;
}

static int32_t
null_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	return 0;
}

static int32_t
invalid_write(file_desc_t* file_desc, const void* buf, int32_t nbytes) {
	return -1;
}

file_ops_t file_ops[NUM_FILE_TYPES] = {
	{ rtc_open, rtc_close, rtc_read, rtc_write },
	{ null_open_close, null_open_close, dir_read, invalid_write },
	{ null_open_close, null_open_close, file_read, invalid_write },
	{ invalid_open_close, invalid_open_close, stdin_read, invalid_write },
	{ invalid_open_close, invalid_open_close, invalid_read, stdout_write },
	{ AdLib_open, AdLib_close, null_read, play_melody }
};


//externally-modified pointer to the file descriptor table
//actual table is in the PCB
//this pointer is updated during every process switch
file_desc_t * file_desc_table;


/* Local functions */
static int32_t find_free_desc(void);
static int32_t desc_is_valid(int32_t fd);


/*
 * open
 *   DESCRIPTION: Opens a file
 *   INPUTS: filename -- the name of the file to open.
 *   OUTPUTS: none.
 *   RETURN VALUE: A file descriptor number on success, -1 on failure.
 *   SIDE EFFECTS: Creates an entry in the file descriptor table.
 *                 Some file types (e.g. RTC) will have additional side effects.
 */
int32_t open(const int8_t* filename) {
	int32_t fd = find_free_desc();
	file_type_t file_type = -1;
	if (fd == -1) {
		// no free descriptors remaining
		return -1;
	}

	file_desc_t* file_desc = &file_desc_table[fd];
	
	//hardcode soundcard open detection, since the current device file in the 
	//		filesystem keeeps getting marked as a normal file, not a device.
	if(strncmp(filename, (int8_t *)"AdLib", 5) == 0)
		file_type = FILE_TYPE_ADLIB;
	else
		file_type = filesystem_open(filename, file_desc);
		
	if (file_type == FILE_TYPE_ERROR) {
		// incorrect filename
		return -1;
	}

	file_desc->file_ops = &file_ops[file_type];
	if (file_desc->file_ops->open() == -1) {
		// for now, this can only happen if opening already open RTC
		return -1;
	}

	file_desc->flags |= DESC_IN_USE;
	return fd;
}


/*
 * close
 *   DESCRIPTION: Closes a file
 *   INPUTS: fd -- the file descriptor to close.
 *   OUTPUTS: none.
 *   RETURN VALUE: 0 on success, -1 on failure.
 *   SIDE EFFECTS: Marks a file desciptor table entry as available.
 *                 Some file types (e.g. RTC) will have additional side effects.
 */
int32_t close(int32_t fd) {
	if (!desc_is_valid(fd)) {
		return -1;
	}

	file_desc_t* file_desc = &file_desc_table[fd];
	if (file_desc->file_ops->close() == -1) {
		// can only happen if closing stdin or stdout
		return -1;
	}

	file_desc->flags &= ~DESC_IN_USE;
	return 0;
}


/*
 * read
 *   DESCRIPTION: Reads from a file
 *   INPUTS: fd -- the file descriptor to read from.
 *           nbytes -- the number of bytes to read.
 *   OUTPUTS: buf -- the read bytes.
 *   RETURN VALUE: The number of bytes read, or -1 on failure.
 *   SIDE EFFECTS: Advances an internal counter for files and directories.
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
	if (!desc_is_valid(fd)) {
		return -1;
	}

	file_desc_t* file_desc = &file_desc_table[fd];
	return file_desc->file_ops->read(file_desc, buf, nbytes);
}


/*
 * write
 *   DESCRIPTION: Writes to a file
 *   INPUTS: fd -- the file descriptor to write to.
 *           buf -- the data to write.
 *           nbytes -- the number of bytes to write.
 *   OUTPUTS: none.
 *   RETURN VALUE: The number of bytes written, or -1 on failure.
 *   SIDE EFFECTS: Writes to the terminal for stdout descriptor.
 *                 Changes the RTC frequency for RTC descriptor.
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
	if (!desc_is_valid(fd)) {
		return -1;
	}

	file_desc_t* file_desc = &file_desc_table[fd];
	return file_desc->file_ops->write(file_desc, buf, nbytes);
}


/*
 * find_free_desc
 *   DESCRIPTION: Finds a free file descriptor
 *   INPUTS: none.
 *   OUTPUTS: none.
 *   RETURN VALUE: A free file descriptor, or -1 if there aren't any.
 *   SIDE EFFECTS: none.
 */
static int32_t find_free_desc(void) {
	int fd;
	// first two descriptors are always open
	for (fd = 2; fd < MAX_OPEN_FILES; ++fd) {
		if (!(file_desc_table[fd].flags & DESC_IN_USE)) {
			return fd;
		}
	}

	// no open descriptors
	return -1;
}


/*
 * desc_is_valid
 *   DESCRIPTION: Checks the validity of a file descriptor
 *   INPUTS: fd -- the file descriptor to check.
 *   RETURN VALUE: 1 if descriptor is in range and in use, 0 otherwise.
 *   SIDE EFFECTS: none.
 */
static int32_t desc_is_valid(int32_t fd) {
	return fd >= 0 && fd < MAX_OPEN_FILES &&
		(file_desc_table[fd].flags & DESC_IN_USE);
}
