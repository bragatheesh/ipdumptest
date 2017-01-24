#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_SIZE 50
#define NUM_CLIENT 1


void hexDump(char *desc, void *addr, int len) {
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	// Output description if given.
	if (desc != NULL)
		printf("%s:\n", desc);

	if (len == 0) {
		printf("  ZERO LENGTH\n");
		return;
	}
	if (len < 0) {
		printf("  NEGATIVE LENGTH: %i\n", len);
		return;
	}

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
}

void *server_handler(void* arg)
{
	int threadnum = (int)pthread_self();
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;
	char client_message[1024];

	struct msghdr msg;
	struct cmsghdr *cmsg;
	
	ssize_t n;
	int data;
	int *dt;

	memset(&msg, 0, sizeof(msg));
	memset(&client_message, '\0', sizeof(client_message));
	socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_desc == -1) {
		printf("could not create server socket\n");
		return NULL;
	}
	printf("socket created\n");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(5080);

	if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
		printf("bind failled\n");
		return NULL;
	}
	printf("Bind done\n");

	//listen(socket_desc, 1);

	//printf("Waiting for incoming connections...\n");
	
	c = sizeof(struct sockaddr_in);

	//client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
	
	/*if (client_sock < 0) {
		printf("accept failed\n");
		return NULL;
	}
	printf("connection accepted\n");
	*/	

	printf("recving msg\n");
	n = recvfrom(client_sock, client_message, sizeof(client_message),0,NULL,0);
	//n = recvmsg(client_sock, &msg, MSG_WAITALL);
	
	/*while(recvmsg(client_sock, &msg, MSG_WAITALL) > 0){
		hexDump("in while",&msg, sizeof(msg));
	}*/

	//printf("msg rcvd size %d calling hex dump\n\n", (int)n);
	hexDump("a test", &client_message, sizeof(client_message));

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL)
	{
		printf("cmsgptr is null\n");
		close(client_sock);
		close(socket_desc);
		return NULL;
	}

	close(client_sock);
	close(socket_desc);
	return NULL;

}

void *client_handler(void *arg)
{
	int threadnum = (int)pthread_self();
	int socket_desc;
	struct sockaddr_in serv_addr;

	static char hello[] = "Hello world";
	struct iovec iov[1];


	struct msghdr msg = {0};          /* Message header  */
	struct cmsghdr *cmsg; /* Ptr to ancillary hdr  */
	int fd = 1234;        /* File descriptor to send */
	char buf[CMSG_SPACE(sizeof fd)];  /* Anc. buf  */
	int *fd_ptr;         /* Ptr to file descriptor */

	
	if ((socket_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		printf("Failed creating socket\n");

	bzero((char *)&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(8889);

	/*if (connect(socket_desc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Failed to connect to server\n");
		return NULL;
	}*/

	//printf("Connected successfully client:%d\n", threadnum);



	msg.msg_name = (void*)&serv_addr;
	msg.msg_namelen = sizeof(serv_addr);


	iov[0].iov_base = hello;
	iov[0].iov_len = strlen(hello);
	
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;	

	msg.msg_control = buf;
	msg.msg_controllen = sizeof (buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));

	/* Initialize the payload: */
	fd_ptr = (int *)CMSG_DATA(cmsg);
	*fd_ptr = fd;

	printf("fd_ptr: %d\n\n", *(int *)CMSG_DATA(cmsg));

/*
	* Sum of the length of all control
	* messages in the buffer:
	*/
	msg.msg_controllen = cmsg->cmsg_len;
	
	//hexDump("from client", &msg, sizeof(msg));
	sendmsg(socket_desc, &msg, 0);
	//hexDump("after send", &msg, sizeof(msg));

	sleep(3);
	close(socket_desc);
	return 0;
}


int main()
{
  int socket_desc , new_socket , c , *new_sock, i;
  pthread_t sniffer_thread, reader_thread;
  for (i=1; i<=NUM_CLIENT; i++) {

    
/*	if (pthread_create(&reader_thread, NULL, server_handler, (void*)NULL) < 0)
	{
		perror("could not create server thread");
		return 1;
	}
	sleep(5);*/
	if (pthread_create(&sniffer_thread, NULL, client_handler, (void*)NULL) < 0)
	{
		perror("could not create client thread");
		return 1;
	}

  }
  pthread_exit(NULL);
  return 0;
}

/*junk



while (n = recv(client_sock, client_message, sizeof(client_message), 0) > 0) {
printf("Message: %s\n", client_message);

if (n == 0) {
printf("transmission done\n");
}
}


iov[0].iov_base = buf;
iov[0].iov_len = sizeof(buf);
msg_hdr.msg_name = (void*)&client;
msg_hdr.msg_namelen = (socklen_t)sizeof(client);
msg_hdr.msg_iov = iov;
msg_hdr.msg_iovlen = 1;
msg_hdr.msg_control = &cmsg;
msg_hdr.msg_controllen = sizeof(cmsg);



msg_hdr.msg_name = (void*)&client;
msg_hdr.msg_namelen = (socklen_t)sizeof(client);
msg_hdr.msg_iov = iov;
msg_hdr.msg_iovlen = 1;
msg_hdr.msg_control = &cmsg;
msg_hdr.msg_controllen = sizeof(cmsg);






//fgets(sbuff, MAX_SIZE, hello);
//send(socket_desc, hello, strlen(hello), 0);



iov[0].iov_base = hello;
iov[0].iov_len = sizeof(hello);

msg.msg_iov = iov;
msg.msg_iovlen = 1;			//sizeof(iov);





for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
printf("In forloop\n");
if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
dt = (int *)CMSG_DATA(cmsg);
data = *dt;
printf("Message: %d\n", data);
break;
}
}











*/
