#!/usr/bin/python
import os
import sys

DIR = "/workdir/mp3/student-distrib"
GDB_FILE = "gdbcommands.txt"

gdb_fh = open( os.path.join(DIR, GDB_FILE), "w" )
gdb_fh.write("target remote 10.0.2.2:1234\n")

def add_breakpoints(dir, file):
	line_num = 0
	for line in open( os.path.join(dir, file) ):
		if "//BREAK" in line:
			gdb_fh.write( "break " + file + ":" + str(line_num) + "\n" )
		# if "//DISP" in line:
			# line_list = line.split()
			# gdb_fh.write( "disp " + line_list[ line_list.index("//DISP") + 1 ] + "\n" )
		line_num += 1
		
	
for root, _, files in os.walk(DIR):
    for f in files:
		if f.endswith(".c") or f.endswith(".S") or f.endswith(".h"):
			add_breakpoints(root, f)

gdb_fh.write("continue\n")