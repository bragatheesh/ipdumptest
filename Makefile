CC = gcc

all: 
	gcc -pthread hellocmsg.c -o hellocmsg
	gcc hellodump.c -o hellodump

clean:
	$(RM) hellocmsg hellodump
