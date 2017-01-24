CC = gcc

all: 
	gcc -pthread hellocmsg.c -o hellocmsg
	gcc hellodump.c -o hellodump
	gcc -pthread mthread.c -o mthread
clean:
	$(RM) hellocmsg hellodump mthread
