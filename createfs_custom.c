#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Filesystem constants and structures */

#define MAX_FILE_NAME_LEN     32
#define MAX_DIR_ENTRIES       63
#define BLOCK_SIZE          4096
#define MAX_BLOCKS_IN_INODE ((BLOCK_SIZE / 4) - 1)
#define MAX_FILE_SIZE       (BLOCK_SIZE * MAX_BLOCKS_IN_INODE)

typedef enum {
	FT_RTC,
	FT_DIR,
	FT_FILE
} file_type_t;

typedef struct {
	char file_name[MAX_FILE_NAME_LEN];
	file_type_t file_type;
	unsigned inode_num;
	char reserved[24];
} dir_entry_t;

typedef struct {
	unsigned num_dir_entries;
	unsigned num_inodes;
	unsigned num_data_blocks;
	char reserved[52];
	dir_entry_t dir_entries[MAX_DIR_ENTRIES];
} boot_block_t;

typedef struct {
	unsigned file_size;
	unsigned block_nums[MAX_BLOCKS_IN_INODE];
} inode_t;


/* List data structure and functions */

typedef struct list_entry {
	struct list_entry* next;
	void* data;
} list_entry_t;

typedef struct {
	list_entry_t* head;
	list_entry_t* tail;
} list_t;

list_t* create_list(void) {
	// automatically sets head and tail to NULL
	return calloc(1, sizeof(list_t));
}

void delete_list(list_t* list, void (*delete_data)(void*)) {
	list_entry_t* entry = list->head;
	while (entry != NULL) {
		list_entry_t* next_entry = entry->next;
		delete_data(entry->data);
		free(entry);
		entry = next_entry;
	}
	free(list);
}

void insert_at_tail(list_t* list, void* data) {
	list_entry_t* entry = malloc(sizeof(list_entry_t));
	entry->next = NULL;
	entry->data = data;
	if (list->tail != NULL) {
		list->tail->next = entry;
	} else {
		list->head = entry; // first entry in list is both head and tail
	}
	list->tail = entry;
}


/* Filesystem creation structures and functions */

typedef struct {
	inode_t inode;
	void* block_ptrs[MAX_BLOCKS_IN_INODE];
	dir_entry_t* dir_entry_ptr;
	unsigned num_data_blocks;
	unsigned inode_num;
} inode_holder_t;

typedef struct {
	unsigned num_inodes;
	unsigned num_data_blocks;
	unsigned char* free_inodes; // 0 if free, 1 if not
	unsigned char* free_data_blocks; // as above
} free_map_t;


char* get_output_file_name(int argc, char* argv[]) {
	if (argc == 2) {
		return "fs.out";
	} else if (argc == 4 && strcmp(argv[2], "-o") == 0) {
		return argv[3];
	} else {
		return NULL;
	}
}


int open_output_file(char* output_file_name) {
	int output_fd = open(output_file_name, O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (output_fd == -1) {
		fprintf(stderr, "open: Output file %s exists, do you want to overwrite?\n(y/n): ",
				output_file_name);
		if (getchar() != 'n') {
			output_fd = open(output_file_name, O_WRONLY | O_TRUNC);
		}
	}
	return output_fd;
}


void init_boot_block(boot_block_t* boot_block) {
	memset(boot_block, 0, sizeof(boot_block_t));
	strcpy(boot_block->dir_entries[0].file_name, ".");
	boot_block->dir_entries[0].file_type = FT_DIR;
	boot_block->num_dir_entries = 1;
}


int create_dir_entry(dir_entry_t* dir_entry, char* entry_name, struct stat* entry_stat) {
	// directory entry is initially zeroed out because entire boot block is
	if (S_ISREG(entry_stat->st_mode)) {
		dir_entry->file_type = FT_FILE;
	} else if (S_ISCHR(entry_stat->st_mode)) {
		dir_entry->file_type = FT_RTC;
	} else {
		return -1;
	}

	// BUG: should be MAX_FILE_NAME_LEN (subtracting 1 is incorrect)
	strncpy(dir_entry->file_name, entry_name, MAX_FILE_NAME_LEN - 1);
	return 0;
}


inode_holder_t* create_file_holder(char* entry_path, struct stat* entry_stat) {
	if (entry_stat->st_size > MAX_FILE_SIZE) {
		return NULL;
	}

	inode_holder_t* file_holder = calloc(1, sizeof(inode_holder_t));
	file_holder->inode.file_size = entry_stat->st_size;
	file_holder->num_data_blocks = (entry_stat->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int entry_fd = open(entry_path, O_RDONLY);
	unsigned i;
	for (i = 0; i < file_holder->num_data_blocks; ++i) {
		file_holder->block_ptrs[i] = calloc(1, BLOCK_SIZE);
		if (read(entry_fd, file_holder->block_ptrs[i], BLOCK_SIZE) == -1) {
			int j;
			for (j = 0; j <= i; ++j) {
				free(file_holder->block_ptrs[j]);
			}
			free(file_holder);
			close(entry_fd);
			return NULL;
		}
	}

	close(entry_fd);
	return file_holder;
}


inode_holder_t* create_device_holder(void) {
	return calloc(1, sizeof(inode_holder_t));
}


void free_inode_holder(void* holder) {
	inode_holder_t* inode_holder = holder;
	unsigned i;
	for (i = 0; i < inode_holder->num_data_blocks; ++i) {
		free(inode_holder->block_ptrs[i]);
	}
	free(inode_holder);
}


list_t* read_fs_dir(char* fs_dir_name, boot_block_t* boot_block) {
	list_t* inode_holders = create_list();
	DIR* fs_dir = opendir(fs_dir_name);
	if (fs_dir == NULL) {
		// WEIRDNESS: for non-existent directory, createfs creates filesystem
		// with only boot block instead of not producing any output
		fprintf(stderr, "opendir: Directory %s does not exist\n", fs_dir_name);
		closedir(fs_dir);
		return inode_holders;
	}

	unsigned cur_dir_entry = boot_block->num_dir_entries;
	errno = 0;
	while (cur_dir_entry < MAX_DIR_ENTRIES) {
		struct dirent* fs_dir_entry = readdir(fs_dir);
		if (fs_dir_entry == NULL) {
			if (errno != 0) {
				perror("readdir");
				closedir(fs_dir);
				delete_list(inode_holders, free_inode_holder);
				return NULL;
			}
			break; // reached end of directory
		}

		char* entry_path = malloc(strlen(fs_dir_name) + 1 + strlen(fs_dir_entry->d_name) + 1);
		strcpy(entry_path, fs_dir_name);
		strcat(entry_path, "/");
		strcat(entry_path, fs_dir_entry->d_name);
		struct stat entry_stat;
		if (stat(entry_path, &entry_stat) != 0) {
			perror("stat");
			closedir(fs_dir);
			delete_list(inode_holders, free_inode_holder);
			return NULL;
		}

		dir_entry_t* dir_entry_ptr = &boot_block->dir_entries[cur_dir_entry];
		inode_holder_t* inode_holder = NULL;
		if (create_dir_entry(dir_entry_ptr, fs_dir_entry->d_name, &entry_stat) == 0) {
			if (dir_entry_ptr->file_type == FT_FILE) {
				inode_holder = create_file_holder(entry_path, &entry_stat);
			} else {
				inode_holder = create_device_holder();
			}
		}
		if (inode_holder != NULL) {
			inode_holder->dir_entry_ptr = dir_entry_ptr;
			boot_block->num_data_blocks += inode_holder->num_data_blocks;
			insert_at_tail(inode_holders, inode_holder);
			++boot_block->num_inodes;
			++cur_dir_entry;
		} else {
			fprintf(stderr, "Could not create an entry for %s, skipping it...\n", entry_path);
		}
		free(entry_path);
	}

	closedir(fs_dir);
	boot_block->num_dir_entries = cur_dir_entry;
	return inode_holders;
}


free_map_t* create_free_map(unsigned num_inodes, unsigned num_data_blocks) {
	free_map_t* free_map = malloc(sizeof(free_map_t));
	free_map->num_inodes = num_inodes * 2;
	free_map->num_data_blocks = num_data_blocks * 2;
	free_map->free_inodes = calloc(free_map->num_inodes, 1);
	free_map->free_data_blocks = calloc(free_map->num_data_blocks, 1);
	return free_map;
}


void delete_free_map(free_map_t* free_map) {
	free(free_map->free_inodes);
	free(free_map->free_data_blocks);
	free(free_map);
}


unsigned get_free_entry(unsigned num_entries, unsigned char* free_entries) {
	while (1) {
		unsigned entry = rand() % num_entries;
		if (!free_entries[entry]) {
			free_entries[entry] = 1;
			return entry;
		}
	}
}


void process_inodes(list_t* inode_holders, free_map_t* free_map) {
	list_entry_t* e;
	for (e = inode_holders->head; e != NULL; e = e->next) {
		inode_holder_t* inode_holder = e->data;
		unsigned inode_num = get_free_entry(free_map->num_inodes, free_map->free_inodes);
		inode_holder->inode_num = inode_num;
		inode_holder->dir_entry_ptr->inode_num = inode_num;
	}
}


void process_data_blocks(list_t* inode_holders, free_map_t* free_map) {
	list_entry_t* e;
	for (e = inode_holders->head; e != NULL; e = e->next) {
		inode_holder_t* inode_holder = e->data;
		unsigned i;
		for (i = 0; i < inode_holder->num_data_blocks; ++i) {
			unsigned block_num = get_free_entry(free_map->num_data_blocks,
					free_map->free_data_blocks);
			inode_holder->inode.block_nums[i] = block_num;
		}
	}
}


int write_boot_block(int output_fd, boot_block_t* boot_block) {
	if (write(output_fd, boot_block, sizeof(boot_block_t)) != sizeof(boot_block_t)) {
		perror("write");
		return -1;
	}
	return 0;
}


int write_inodes(int output_fd, list_t* inode_holders) {
	list_entry_t* e;
	for (e = inode_holders->head; e != NULL; e = e->next) {
		inode_holder_t* inode_holder = e->data;
		off_t offset = sizeof(boot_block_t) + inode_holder->inode_num * sizeof(inode_t);
		if (lseek(output_fd, offset, SEEK_SET) == -1) {
			perror("lseek");
			return -1;
		}
		if (write(output_fd, &inode_holder->inode, sizeof(inode_t)) != sizeof(inode_t)) {
			perror("write");
			return -1;
		}
	}
	return 0;
}


int write_data_blocks(int output_fd, list_t* inode_holders, unsigned num_inodes) {
	list_entry_t* e;
	for (e = inode_holders->head; e != NULL; e = e->next) {
		inode_holder_t* inode_holder = e->data;
		unsigned i;
		for (i = 0; i < inode_holder->num_data_blocks; ++i) {
			off_t offset = sizeof(boot_block_t) + num_inodes * sizeof(inode_t) +
				inode_holder->inode.block_nums[i] * BLOCK_SIZE;
			if (lseek(output_fd, offset, SEEK_SET) == -1) {
				perror("lseek");
				return -1;
			}
			if (write(output_fd, inode_holder->block_ptrs[i], BLOCK_SIZE) != BLOCK_SIZE) {
				perror("write");
				return -1;
			}
		}
	}
	return 0;
}


int main(int argc, char* argv[]) {
	char* output_file_name = get_output_file_name(argc, argv);
	if (output_file_name == NULL) {
		fprintf(stderr, "Usage: fscreate <directory> [-o <output file>]\n");
		return EXIT_FAILURE;
	}

	int output_fd = open_output_file(output_file_name);
	if (output_fd == -1) {
		return EXIT_FAILURE;
	}

	boot_block_t boot_block;
	init_boot_block(&boot_block);

	list_t* inode_holders = read_fs_dir(argv[1], &boot_block);
	if (inode_holders == NULL) {
		close(output_fd);
		return EXIT_FAILURE;
	}

	free_map_t* free_map = create_free_map(boot_block.num_inodes, boot_block.num_data_blocks);
	boot_block.num_inodes = free_map->num_inodes;
	boot_block.num_data_blocks = free_map->num_data_blocks;

	process_inodes(inode_holders, free_map);
	process_data_blocks(inode_holders, free_map);
	delete_free_map(free_map);

	int exit_status = EXIT_FAILURE;
	if (write_boot_block(output_fd, &boot_block) == 0) {
		if (write_inodes(output_fd, inode_holders) == 0) {
			if (write_data_blocks(output_fd, inode_holders, boot_block.num_inodes) == 0) {
				exit_status = EXIT_SUCCESS;
			}
		}
	}

	delete_list(inode_holders, free_inode_holder);
	close(output_fd);
	return exit_status;
}
