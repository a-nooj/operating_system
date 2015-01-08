#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd, i;

    for (i = 0; i < 16; i++) {
	    if (-1 == (fd = ece391_open ((uint8_t*)"."))) {
            ece391_fdputs (1, (uint8_t*)"err_open_lots: directory open failed on attempt #%d \n", i);
            return 2;
        }
    }

    return 0;
}
