ALL: cat grep hello ls pingpong sched shell sigtest testprint errchk music colors

%.o: %.c
	gcc -c -Wall -o $@ $<

%.o: %.S
	gcc -c -Wall -o $@ $<

cat.exe: ece391cat.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o cat.exe ece391cat.o ece391syscall.o ece391support.o
cat: cat.exe
	../elfconvert cat.exe
	mv cat.exe.converted to_fsdir/cat

grep.exe: ece391grep.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o grep.exe ece391grep.o ece391syscall.o ece391support.o
grep: grep.exe
	../elfconvert grep.exe
	mv grep.exe.converted to_fsdir/grep

hello.exe: ece391hello.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o hello.exe ece391hello.o ece391syscall.o ece391support.o
hello: hello.exe
	../elfconvert hello.exe
	mv hello.exe.converted to_fsdir/hello

ls.exe: ece391ls.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o ls.exe ece391ls.o ece391syscall.o ece391support.o
ls: ls.exe
	../elfconvert ls.exe
	mv ls.exe.converted to_fsdir/ls

pingpong.exe: ece391pingpong.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o pingpong.exe ece391pingpong.o ece391syscall.o ece391support.o
pingpong: pingpong.exe
	../elfconvert pingpong.exe
	mv pingpong.exe.converted to_fsdir/pingpong

sched.exe: ece391sched.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o sched.exe ece391sched.o ece391syscall.o ece391support.o
sched: sched.exe
	../elfconvert sched.exe
	mv sched.exe.converted to_fsdir/sched

shell.exe: ece391shell.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o shell.exe ece391shell.o ece391syscall.o ece391support.o
shell: shell.exe
	../elfconvert shell.exe
	mv shell.exe.converted to_fsdir/shell

sigtest.exe: ece391sigtest.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o sigtest.exe ece391sigtest.o ece391syscall.o ece391support.o
sigtest: sigtest.exe
	../elfconvert sigtest.exe
	mv sigtest.exe.converted to_fsdir/sigtest
	
testprint.exe: ece391testprint.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o testprint.exe ece391testprint.o ece391syscall.o ece391support.o
testprint: testprint.exe
	../elfconvert testprint.exe
	mv testprint.exe.converted to_fsdir/testprint

errchk.exe: ece391errchk.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o errchk.exe ece391errchk.o ece391syscall.o ece391support.o
errchk: errchk.exe
	../elfconvert errchk.exe
	mv errchk.exe.converted to_fsdir/errchk
	
music.exe: ece391music.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o music.exe ece391music.o ece391syscall.o ece391support.o
music: music.exe
	../elfconvert music.exe
	mv music.exe.converted to_fsdir/music
	
colors.exe: ece391colors.o ece391syscall.o ece391emulate.o ece391support.o
	gcc -nostdlib -o colors.exe ece391colors.o ece391syscall.o ece391support.o
colors: colors.exe
	../elfconvert colors.exe
	mv colors.exe.converted to_fsdir/colors	

	
clean::
	rm -f *~ *.o

clean::
	rm -f *~ *.o

clear: clean
	rm -f *.converted
	rm -f *.exe
	rm -f to_fsdir/*

