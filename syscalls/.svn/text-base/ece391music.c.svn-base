#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
	int fd = -1;


    if (-1 == (fd = ece391_open ("AdLib"))) {
        ece391_fdputs (1, (uint8_t*)"Device not found\n");
		return 2;
    }
	
	
	//A major scale
	ece391_fdputs (1, (uint8_t*)"A Major Scale\n");
	if (-1 == ece391_write (fd, "500|AN42a|BN42a|C#52a|DN52a|EN52a|F#52a|G#52a|AN53a|RN54ax",58)){
	    return 3;
    }
	//Bb major scale
	ece391_fdputs (1, (uint8_t*)"Bb Major Scale\n");
	if (-1 == ece391_write (fd,"400|Bb22a|CN32a|DN32a|Eb32a|FN32a|GN32a|AN32a|Bb33a|RN36ax",58)){
	    return 3;
    }
	//C Arpeggiated Chords
	ece391_fdputs (1, (uint8_t*)"I-IV-V-I Chord progression With different Voices\n");
	if (-1 == ece391_write (fd, "800|CN41a|EN41a|GN41a|EN41a|CN43a|RN33a|CN41b|FN41b|AN41b|FN41b|CN43b|RN33a|DN41c|GN41c|BN41c|GN41c|DN42c|RN22a|CN41d|EN41d|GN41d|EN41d|CN45d|RN34dx",148)){
	    return 3;
    }
	
	if (-1 == ece391_close (fd)) {
        ece391_fdputs (1, (uint8_t*)"Device close failed\n");
        return -1;
    }
	
	ece391_fdputs (1, (uint8_t*)"Done!\n\n");
	return 0;
}
