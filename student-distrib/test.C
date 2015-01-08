#include "test.h"
#include "filesystem.h"
#include "files.h"

// Since tests are run one after another, it is important for a successful test to clean up after itself,
// otherwise it becomes very difficult to isolate failures. Unsuccessful tests cause termination, however,
// so don't worry about something not getting cleaned up in case of test failure.

// the parameter given to S_TEST becomes the function name, so you can use it to set breakpoints

/* Tests for open */
S_TEST(OpenValidFile) {
	int32_t fd = open("frame0.txt");
	S_ASSERT(fd != -1, "Opening a valid file failed\n");
	close(fd);
}

S_TEST(OpenInvalidFile) {
	int32_t fd = open("foobarbaz.txt");
	S_ASSERT(fd == -1, "I was able to open a non-existent file\n");
}

S_TEST(OpenDirectory) {
	int32_t fd = open(".");
	S_ASSERT(fd != -1, "Opening the directory failed\n");
	close(fd);
}

S_TEST(OpenRTC) {
	int32_t fd = open("rtc");
	S_ASSERT(fd != -1, "Opening the RTC failed\n");
	close(fd);
}


/* Tests for close */
S_TEST(CloseOpenDescriptor) {
	int32_t fd = open("frame0.txt");
	int32_t retval = close(fd);
	S_ASSERT(retval == 0, "Closing an open descriptor failed");
}

S_TEST(CloseTerminal) {
	int32_t retval = close(0);
	S_ASSERT(retval == -1, "I was able to close stdin\n");
	retval = close(1);
	S_ASSERT(retval == -1, "I was able to close stdout\n");
}

S_TEST(CloseInvalidDescriptor) {
	int32_t retval = close(-5);
	S_ASSERT(retval == -1, "I was able to close a non-existent descriptor\n");
	retval = close(3);
	S_ASSERT(retval == -1, "I was able to close an unopened descriptor\n");
}


/* Tests for open and close in conjunction */
S_TEST(MaxOutDescriptors) {
	int32_t fd[6];
	int i;
	for (i = 0; i < 6; ++i) {
		fd[i] = open("frame0.txt");
		S_ASSERT(fd[i] != -1, "Opening a valid file failed\n");
	}

	// at this point, all 8 descriptors should be in use
	int32_t fail_fd = open("frame0.txt");
	S_ASSERT(fail_fd == -1, "I was able to open more than 8 descriptors\n");
	close(fd[0]);
	fd[0] = open("frame0.txt");
	S_ASSERT(fd[0] != -1, "Closing a file descriptor didn't reclaim it\n");

	for (i = 0; i < 6; ++i) {
		close(fd[i]);
	}
}

S_TEST(OpenRTCTwice) {
	int32_t fd = open("rtc");
	int32_t fail_fd = open("rtc");
	S_ASSERT(fail_fd == -1, "I was able to open the RTC twice\n");
	close(fd);
	fd = open("rtc");
	S_ASSERT(fd != -1, "Closing the RTC didn't mark it as free\n");
	close(fd);
}


/* Tests for read */
S_TEST(ReadFromInvalidDescriptors) {
	char buf[10];
	int32_t bytes_read = read(2, buf, 10);
	S_ASSERT(bytes_read == -1, "I was able to read from a closed descriptor\n");
	int32_t fd = open("grep");
	close(fd);
	bytes_read = read(fd, buf, 10);
	S_ASSERT(bytes_read == -1, "I was able to read from a descriptor I just closed\n");
	bytes_read = read(79, buf, 10);
	S_ASSERT(bytes_read == -1, "I was able to read from a non-existent descriptor\n");
}

// tests which take user input are annoying to run over and over again
// set RUN_IGNORED_TESTS to 1 in generate_tests if you wanna run them
S_IGNORE(ReadFromTerminalShort) {
	int8_t buf[10];
	buf[2] = -1; // to do bounds checking
	S_PRINT("Please enter a character and then press Enter\n");
	int32_t bytes_read = read(0, buf, 10);
	S_ASSERT(bytes_read == 2, "Please follow instructions ... I even asked nicely\n");
	S_ASSERT(buf[1] == '\n', "The newline character wasn't read in\n");
	S_ASSERT(buf[2] == -1, "read wrote more than it was supposed to\n");
}

S_IGNORE(ReadFromTerminalLong) {
	int8_t buf[12];
	buf[10] = 0;
	buf[11] = -1; // to do bounds checking
	S_PRINT("Please enter the following line and then press enter\n");
	int8_t* line = "The quick brown fox jumps over the lazy dog\n";
	S_PRINT(line);
	int32_t bytes_read = read(0, buf, 10);
	S_ASSERT(bytes_read == 10, "I didn't read in 10 characters\n");
	// SPEC AMBIGUITY: are we supposed to be reading in the newline here?
	S_ASSERT(strncmp(line, buf, 9) == 0 && buf[9] == '\n', "Mismatch between input text and read text\n");
}

S_IGNORE(ReadFromTerminalWeirdParameters) {
	int8_t buf[3] = { -1, -1, -1 }; // initialize
	int32_t bytes_read = read(0, buf + 1, -1);
	S_ASSERT(bytes_read == -1, "Reading -1 bytes from the terminal did something weird\n");
	S_ASSERT(buf[0] == -1 && buf[1] == -1 && buf[2] == -1,
			"Reading -1 bytes from the terminal actually wrote to the buffer\n");
	bytes_read = read(0, buf + 1, 0);
	S_ASSERT(bytes_read == 0, "Reading 0 bytes from the terminal did something weird\n");
	S_ASSERT(buf[0] == -1 && buf[1] == -1 && buf[2] == -1,
			"Reading 0 bytes from the terminal actually wrote to the buffer\n");
	S_PRINT("Press enter to continue test");
	bytes_read = read(0, buf + 1, 1);
	S_ASSERT(bytes_read == 1, "Reading 1 byte from the terminal did something weird\n");
	S_ASSERT(buf[0] == -1 && buf[1] == '\n' && buf[2] == -1,
			"Reading 1 byte from the terminal wrote more than it was supposed to\n");
}

static char* dir_entries[] = {
	".",
	"frame1.txt",
	"verylargetxtwithverylongname.tx",
	"ls",
	"grep",
	"sched",
	"hello",
	"rtc",
	"testprint",
	"sigtest",
	"shell",
	"fish",
	"cat",
	"frame0.txt",
	"pingpong"
};
static uint32_t num_dir_entries = sizeof(dir_entries) / sizeof(char*);

static uint8_t* file_data[] = {
	NULL,
	frame1_data,
	verylarge_data,
	ls_data,
	grep_data,
	sched_data,
	hello_data,
	NULL,
	testprint_data,
	sigtest_data,
	shell_data,
	fish_data,
	cat_data,
	frame0_data,
	pingpong_data
};

// I'm making this an array of pointers because gcc (rightfully) complains about
// non-constant initializer elements otherwise. Hacky but it works.
// The alternative would be moving the array inside the function which needs it,
// but that really clutters up the function
static uint32_t* file_sizes[] = {
	NULL,
	&frame1_size,
	&verylarge_size,
	&ls_size,
	&grep_size,
	&sched_size,
	&hello_size,
	NULL,
	&testprint_size,
	&sigtest_size,
	&shell_size,
	&fish_size,
	&cat_size,
	&frame0_size,
	&pingpong_size
};

S_TEST(ReadFromDirectoryNormal) {
	char buf[MAX_NAME_LEN];
	int32_t fd = open(".");
	int i;
	for (i = 0; i < num_dir_entries; ++i) {
		int32_t bytes_read = read(fd, buf, MAX_NAME_LEN);
		// SPEC AMBIGUITY: I'm not sure if this is the correct behavior
		// the spec says "only the filename should be provided (as much as fits, or all 32 bytes)"
		// which makes it sound like, no matter how small the filename, always copy all 32 bytes
		S_ASSERT(bytes_read == MAX_NAME_LEN,
				"Reading from the directory didn't read the entire filename in\n");
		S_ASSERT(strncmp(dir_entries[i], buf, bytes_read) == 0,
				"Mismatch between directory entry and read entry\n");
	}

	// make sure nothing untoward happens when reading past end of directory
	for (i = 0; i < 5; ++i) {
		S_ASSERT(read(fd, buf, 33) == 0, "Reading past end of directory did something weird\n");
	}

	close(fd);
}

S_TEST(ReadFromDirectoryShortBuffer) {
	char buf[5];
	buf[4] = -1; // for bounds checking
	int32_t fd = open(".");

	int i;
	for (i = 0; i < num_dir_entries; ++i) {
		int32_t bytes_read = read(fd, buf, 4);
		S_ASSERT(bytes_read == 4,
				"Reading from the directory read in more than it was supposed to\n");
		S_ASSERT(buf[4] == -1, "Reading from the directory went past the end of the buffer\n");
		S_ASSERT(strncmp(dir_entries[i], buf, bytes_read) == 0,
				"Mismatch between directory entry and read entry\n");
	}

	close(fd);
}

S_TEST(ReadFromDirectoryWeirdParameters) {
	char buf = -1;
	int32_t fd = open(".");

	int32_t bytes_read = read(fd, &buf, -10);
	S_ASSERT(bytes_read == -1, "Directory read not robust for nonsencical nbytes\n");
	S_ASSERT(buf == -1, "Directory read incorrectly overwrote buffer\n");

	bytes_read = read(fd, &buf, 1);
	S_ASSERT(bytes_read == 1, "Directory read didn't read in 1 byte correctly\n");
	S_ASSERT(buf == dir_entries[0][0], "Nonsensical nbytes incorrectly advanced directory entry\n");

	// SPEC AMBIGUITY: I'm not actually sure if
	// a) 0 byte reads should return 0 or -1
	// b) they should advance the directory entry or not
	// Right now they return 0 but don't advance the directory entry
	buf = -1;
	bytes_read = read(fd, &buf, 0);
	S_ASSERT(bytes_read == 0, "Directory read didn't read in 0 bytes correctly\n");
	S_ASSERT(buf == -1, "Directory read overwrote buffer on 0 byte read\n");

	bytes_read = read(fd, &buf, 1);
	S_ASSERT(buf == dir_entries[1][0], "0 byte directory read advanced directory entry\n");

	close(fd);
}

static uint8_t file_buf[4096 * 9]; // larger than any file

S_TEST(ReadAllFiles) {
	int i;
	clear();
	for (i = 0; i < num_dir_entries; ++i) {
		if (file_data[i] != NULL) {
			int32_t fd = open(dir_entries[i]);
			int32_t bytes_read = read(fd, file_buf, sizeof(file_buf));
			S_ASSERT(bytes_read == *file_sizes[i], "Did not read in entire file\n");
			printf("File %s has size %x\n", dir_entries[i], bytes_read);

			// no memcmp :(
			int j;
			for (j = 0; j < bytes_read; ++j) {
				S_ASSERT(file_buf[j] == file_data[i][j], "File data did not match expected value\n");
			}

			close(fd);
		}
	}
	S_ASSERT(1 == 0, "The universe is in fact sane\n");
}

// parametrized test
static void readFileInChunks(const int8_t* file_name, const uint8_t* file_data,
		uint32_t file_size, uint32_t chunk_size) {
	int32_t fd = open(file_name);
	*((int*) &file_buf[chunk_size]) = 0xcafebabe; // boundary check

	int i;
	for (i = 0; i < file_size; i += chunk_size) {
		int32_t bytes_read = read(fd, file_buf, chunk_size);
		int32_t expected_read = file_size - i < chunk_size ? file_size - i : chunk_size;
		S_ASSERT(bytes_read == expected_read, "Read in an incorrect number of bytes for file\n");

		int j;
		for (j = 0; j < bytes_read; ++j) {
			S_ASSERT(file_buf[j] == *file_data++, "File data did not match expected value\n");
		}
	}

	S_ASSERT(*((int*) &file_buf[chunk_size]) == 0xcafebabe, "Read overflowed its buffer");
	close(fd);
}

S_TEST(ReadFileInChunks) {
	readFileInChunks("fish", fish_data, fish_size, 64); // evenly divides block size
	readFileInChunks("fish", fish_data, fish_size, 100); // does not evenly divide block size
	readFileInChunks("fish", fish_data, fish_size, 4000); // close to a block
	readFileInChunks("fish", fish_data, fish_size, 4096); // a block at a time
	readFileInChunks("fish", fish_data, fish_size, 7000); // between a block and two blocks
	readFileInChunks("fish", fish_data, fish_size, 8192); // two blocks at a time
	readFileInChunks("fish", fish_data, fish_size, 9999); // over two blocks at a time
}

S_TEST(ReadFileWeirdParameters) {
	int32_t fd = open("frame0.txt");
	file_buf[0] = 255;
	file_buf[1] = 255;

	int32_t bytes_read = read(fd, file_buf, -1);
	S_ASSERT(bytes_read == -1, "Reading -1 bytes caused weird things to happen\n");
	bytes_read = read(fd, file_buf, 0);
	S_ASSERT(bytes_read == 0, "Reading 0 bytes didn't work as expected\n");
	S_ASSERT(file_buf[0] == 255, "Reading 0 bytes actually read something in\n");
	bytes_read = read(fd, file_buf, 1);
	S_ASSERT(bytes_read == 1, "Reading 1 byte didn't work as expected\n");
	S_ASSERT(file_buf[0] == frame0_data[0], "Reading 1 byte didn't read the correct byte\n");
	S_ASSERT(file_buf[1] == 255, "Reading 1 byte overflowed the buffer\n");

	close(fd);
}

S_TEST(ReadFilePastEOF) {
	int32_t fd = open("frame1.txt");
	read(fd, file_buf, frame1_size); // get to EOF

	int i;
	for (i = 0; i < 5; ++i) {
		int32_t bytes_read = read(fd, file_buf, 42);
		S_ASSERT(bytes_read == 0, "Reading past EOF didn't behave as expected\n");
	}

	close(fd);
}


/* Tests for write */

// it's kinda stupid to test writing to the terminal, since none of this would have worked otherwise
// nevertheless, we can at least make sure it respects the nbytes parameter and handles odd values correctly
S_TEST(WriteToTerminal) {
	int8_t* str = "foo bar baz\n";
	S_PRINT("The next two lines should match\n");
	int32_t bytes_written = write(1, str, strlen(str));
	S_ASSERT(bytes_written == strlen(str), "Writing to terminal didn't write out everything\n");
	bytes_written = write(1, str, 4);
	S_ASSERT(bytes_written == 4, "Writing to terminal didn't write out everything\n");
	bytes_written = write(1, str + 4, 4);
	S_ASSERT(bytes_written == 4, "Writing to terminal didn't write out everything\n");
	bytes_written = write(1, str + 8, 4);
	S_ASSERT(bytes_written == 4, "Writing to terminal didn't write out everything\n");

	bytes_written = write(1, NULL, 0);
	S_ASSERT(bytes_written == 0, "Writing 0 bytes to terminal did something weird\n");
	bytes_written = write(1, NULL, -128);
	S_ASSERT(bytes_written == -1, "Wrriting -128 bytes to terminal did something weird\n");
}

S_TEST(WriteToFilesystem) {
	// test writing to directories
	int32_t fd = open(".");
	int32_t bytes_written = write(fd, NULL, 10);
	S_ASSERT(bytes_written == -1, "Writing to directory didn't immediately fail\n");
	close(fd);

	fd = open("pingpong");
	bytes_written = write(fd, "pongping", 9);
	S_ASSERT(bytes_written == -1, "Writing to file didn't immediately fail\n");
	close(fd);
}


S_TEST(TestReadRTC) {
	int32_t fd = open("rtc");
	int i;
	// changing from char * to char[] forces stack allocation
	// so each function gets its own copy
	char count_string[] = "1...";

	S_ASSERT(fd != -1, "Opening the RTC failed\n");
	
	S_PRINT("Should count to 5 : ...");
	for (i =0; i < 10; i++) {
		read(fd, NULL, 0);
		if (i%2) {
			S_PRINT(count_string);
			count_string[0]++;
		}
	} S_PRINT("\n");

	close(fd);
}

S_TEST(TestInvalidWriteRTC) {
	int32_t fd = open("rtc");
	int32_t bytes_written;
	int32_t rate;

	S_ASSERT(fd != -1, "Opening the RTC failed!\n");
	
	rate = 0;
	bytes_written = write(fd, (void*)rate, 0);
	S_ASSERT(bytes_written == -1, "Writing RTC rate below minimuum (0) didn't fail!\n");

	rate = 1025;
	bytes_written = write(fd, (void*)rate, 0);
	S_ASSERT(bytes_written == -1, "Writing RTC rate above maximum (1025) didn't fail!\n");	

	rate = 5;
	bytes_written = write(fd, (void*)rate, 0);
	S_ASSERT(bytes_written == -1, "Writing RTC rate to non-power-of-2 (5) didn't fail!\n");	

	rate = 24;
	bytes_written = write(fd, (void*)rate, 0);
	S_ASSERT(bytes_written == -1, "Writing RTC rate to non-power-of-2 (24) didn't fail!\n");	
	
	close(fd);
}

S_TEST(TestValidWriteRTC) {
	int32_t fd = open("rtc");
	int32_t bytes_written;
	int32_t rate;
	int i;
	char count_string[] = "1...";

	S_ASSERT(fd != -1, "Opening the RTC failed!\n");
	
	rate = 16;
	bytes_written = write(fd, (void*)rate, 0);
	S_ASSERT(bytes_written != -1, "Writing RTC rate (16) failed!\n");
	
	S_PRINT("Should to 9 count quickly :  ");
	for (i =0; i < 9; i++) {
		read(fd, NULL, 0);
		S_PRINT(count_string);
		count_string[0]++;
	} S_PRINT("\n");
	
	close(fd);
}

S_TEST(TestOpenRTCSetsDefaultRate) {
	int32_t fd = open("rtc");
	write(fd, (void*) 16, 0);
	close(fd);

	// this should reset the rate to 2 Hz
	fd = open("rtc");
	
	char count_string[] = "1...";
	S_PRINT("Should count to 5 once per second : ...");
	int i;
	for (i =0; i < 10; i++) {
		read(fd, NULL, 0);
		if (i%2) {
			S_PRINT(count_string);
			count_string[0]++;
		}
	} S_PRINT("\n");

	close(fd);
}
