# choose your compiler
CC=gcc
#CC=gcc -Wall

mysh: sh.o get_path.o main.c
	$(CC) -g main.c sh.o get_path.o -o mysh
#	$(CC) -g main.c sh.o get_path.o bash_getcwd.o -o mysh

pathmain: get_path.o get_path_main.o
	$(CC) -g get_path.o get_path_main.o -o pathmain

sh.o: sh.c sh.h
	$(CC) -g -c sh.c

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

get_path_main.o: get_path_main.c get_path.h
	$(CC) -g -c get_path_main.c

clean:
	rm -rf sh.o get_path.o mysh *~ mysh.dSYM *.o pathmain
