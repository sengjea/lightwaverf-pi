#include "lightwaverf.h"
#include <stdlib.h>

byte id[] = {0x6f,0xed,0xbb,0xdb,0x7b,0xee};

int main(int argc, char *argv[]) {

        if (argc != 3 && argc != 4) {
		printf("Usage send2 channel command [level]\n");
		return 1;
	}

	lw_setup();

    	int channel  = atoi(argv[1]);
        int cmd = atoi(argv[2]);
        int level = (argc == 4 ? atoi(argv[3]) : 0);
    
	printf("sending command %d on channel %d level %d\n", cmd, channel, level);

        lw_cmd(level, channel, cmd, id);
    
	return 0;

}
