#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd;
	if (-1 == (fd = ece391_open ((uint8_t*)"helloo"))) {
        ece391_fdputs (1, (uint8_t*)"err_open: file open failed on attempt 'helloo'\n");
		return 2;
    }
	if (-1 == (fd = ece391_open ((uint8_t*)"shel"))) {
        ece391_fdputs (1, (uint8_t*)"err_open: file open failed on attempt 'shel'\n");
		return 2;
    }
	if (-1 == (fd = ece391_open ((uint8_t*)""))) {
        ece391_fdputs (1, (uint8_t*)"err_open: file open failed on empty filename\n");
		return 2;
    }

    return 0;
}
