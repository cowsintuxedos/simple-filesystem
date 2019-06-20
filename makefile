all: fs

fs: fs.c
	gcc -c -g -Wall fs.c -o fs.o