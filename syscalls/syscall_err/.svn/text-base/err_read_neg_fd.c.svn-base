#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t cnt, fd = -15;
	uint8_t buf[32];
	if (-1 == (cnt = ece391_read(fd, buf, 31))) {
		ece391_fdputs (1, (uint8_t*)"err_read_neg_fd: file read failed on fd #%d", fd);
		return 2;
	}
	return 0;
}
