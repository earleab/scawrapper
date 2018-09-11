CC=gcc
FLAGS=-c -g
all: scawrapper.exe

scawrapper.exe: wrapper.o ini.o
	$(CC) -g wrapper.o ini.o -o scawrapper.exe

wrapper.o: wrapper.c
	$(CC) $(FLAGS) wrapper.c
     
ini.o: ini.c
	$(CC) $(FLAGS) ini.c
     
clean:
	rm *.o scawrapper.exe