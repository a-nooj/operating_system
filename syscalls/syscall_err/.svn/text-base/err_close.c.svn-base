#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd, i;
	for (i = 7; i > 1; i--) {
	    if (-1 == (fd = ece391_close ())) {
            ece391_fdputs (1, (uint8_t*)"err_close: file close failed on fd #%d \n", i);
			return 2;
        }
	}

    return 0;
}
