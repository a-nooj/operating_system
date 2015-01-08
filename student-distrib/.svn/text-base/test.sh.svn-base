#!/bin/bash
./gen_gdbcommands.py
if [ $# -eq 1 ]; then
	if [ $1 == "-d" ]; then
		echo "debugging"
		sudo rm -rf /mnt/tmpmp3
		rm bootimg
		rm mp3.img
		svn up mp3.img		
	fi
fi
make dep
sudo make
read -p "Start test machine and press ENTER to continue..."
gdb -x gdbcommands.txt bootimg
#gdb bootimg

