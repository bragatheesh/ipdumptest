#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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

        // Output description if given.^M
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

        // Process every byte in the data.^M
        for (i = 0; i < len; i++) {
                // Multiple of 16 means new line (with line offset).^M

                if ((i % 16) == 0) {
                        // Just don't print ASCII for the zeroth line.^M
                        if (i != 0)
                                printf("  %s\n", buff);

                        // Output the offset.^M
                        printf("  %04x ", i);
                }

                // Now the hex code for the specific character.^M
                printf(" %02x", pc[i]);

                // And store a printable ASCII character for later.^M
                if ((pc[i] < 0x20) || (pc[i] > 0x7e))
                        buff[i % 16] = '.';
                else
                        buff[i % 16] = pc[i];
                buff[(i % 16) + 1] = '\0';
        }

        // Pad out last line if not exactly 16 characters.^M
        while ((i % 16) != 0) {
		printf("   ");
                i++;
        }

        // And print the final ASCII bit.^M
        printf("  %s\n", buff);
}



void *server_handler(void* arg)
{
	int threadnum = (int)pthread_self();
	int socket_desc, client_sock, c, *new_sock, n;
	struct sockaddr_in server, client;
	char client_message[1024];


	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov[1];
	ssize_t nbytes;
	char buf[1024];

	setbuf(stdout, NULL);


	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("could not create server socket\n");
		return NULL;
	}
	printf("socket created\n");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8889);

	if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
		printf("bind failled\n");
		return NULL;
	}
	printf("Bind done\n");

	//here
	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &client;
	msg.msg_namelen = sizeof(client);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &cmsg;
	msg.msg_controllen = sizeof(cmsg);


	listen(socket_desc, 1);

	printf("Waiting for incoming connections...\n");
	
	c = sizeof(struct sockaddr_in);
	
	memset(&client_message, '\0', sizeof(client_message));
	
	client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		printf("accept failed\n");
		return NULL;
	}
	printf("connection accepted\n");

	for(;;){
		printf("recvmsg..");
		nbytes = recvmsg(client_sock, &msg, 0);
		if(nbytes == -1)
			printf("ERROR\n");
		
		printf("message: %ld bytes\n",(long)nbytes);
		hexDump("dump",&msg,sizeof(msg));
		break;
	}


	/*while (n = recv(client_sock, client_message, sizeof(client_message), 0) > 0) {
		printf("Message: %s\n", client_message);
		hexDump("message hex:", &client_message, sizeof(client_message));
		if (n == 0) {
			printf("transmission done\n");
		}
	}*/
	close(client_sock);
	close(socket_desc);
	return NULL;

}

void *client_handler(void *arg)
{
	int threadnum = (int)pthread_self();
	int socket_desc;
	struct sockaddr_in serv_addr;
	char sbuff[MAX_SIZE], rbuff[MAX_SIZE];
	char *msg = "Hello world";

	if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Failed creating socket\n");

	bzero((char *)&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(8889);
	
	//here
	struct iovec iov[1];
	struct msghdr  msgr;
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(msg))];
	char **msg_ptr;
	
	memset(&msgr, 0, sizeof(msgr));

	iov[0].iov_base = msg;
	iov[0].iov_len = strlen(msg);

	msgr.msg_iov = iov;
	msgr.msg_iovlen = 1;

	msgr.msg_control = buf;
	msgr.msg_controllen = sizeof(buf);

	cmsg = CMSG_FIRSTHDR(&msgr);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(char)*11);

	msg_ptr = (char **)CMSG_DATA(cmsg);
	*msg_ptr = msg;

	printf("msg_ptr: %s\n",msg_ptr);

	if (connect(socket_desc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Failed to connect to server\n");
		return NULL;
	}

	printf("Connected successfully client:%d\n", threadnum);
	printf("For thread : %d\n", threadnum);
	//fgets(sbuff, MAX_SIZE, msg);
	//send(socket_desc, msg, strlen(msg), 0);
	//hexDump("client:",msg, sizeof(msg));
	sendmsg(socket_desc, &msgr, 0);
	//hexDump("dump1",&msgr,sizeof(msgr));
	close(socket_desc);
	return 0;
}


int main()
{
  int socket_desc , new_socket , c , *new_sock, i;
  pthread_t sniffer_thread, reader_thread;
  for (i=1; i<=NUM_CLIENT; i++) {

    
	if (pthread_create(&reader_thread, NULL, server_handler, (void*)NULL) < 0)
	{
		perror("could not create server thread");
		return 1;
	}
	sleep(3);
	if (pthread_create(&sniffer_thread, NULL, client_handler, (void*)NULL) < 0)
	{
		perror("could not create client thread");
		return 1;
	}

  }
  pthread_exit(NULL);
  return 0;
}

