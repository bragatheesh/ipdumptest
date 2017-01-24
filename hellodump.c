#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LISTEN_PORT     8889
#define BUFSIZE         1024

#if defined IP_RECVDSTADDR
# define DSTADDR_SOCKOPT IP_RECVDSTADDR
# define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_addr)))
# define dstaddr(x) (CMSG_DATA(x))
#elif defined IP_PKTINFO
# define DSTADDR_SOCKOPT IP_PKTINFO
# define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_pktinfo)))
# define dstaddr(x) (&(((struct in_pktinfo *)(CMSG_DATA(x)))->ipi_addr))
#else
# error "can't determine socket option"
#endif

union control_data {
  struct cmsghdr  cmsg;
  u_char          data[DSTADDR_DATASIZE];
};


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



int
main(void)
{
  int                 sock;
  int                 sockopt;
  struct sockaddr_in  srvaddr;
  struct sockaddr_in  cliaddr;
  struct msghdr       msg;
  union control_data  cmsg;
  struct cmsghdr     *cmsgptr;
  struct iovec        iov[1];
  ssize_t             nbytes;
  char                buf[BUFSIZE];

  setbuf(stdout, NULL);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  sockopt = 1;
  if (setsockopt(sock, IPPROTO_IP, DSTADDR_SOCKOPT, &sockopt, sizeof sockopt) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  memset(&srvaddr, 0, sizeof srvaddr);
  srvaddr.sin_family      = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvaddr.sin_port        = htons(LISTEN_PORT);

  if (bind(sock, (struct sockaddr *)&srvaddr, sizeof srvaddr) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  iov[0].iov_base = buf;
  iov[0].iov_len  = sizeof buf;

  memset(&msg, 0, sizeof msg);
  msg.msg_name       = &cliaddr;
  msg.msg_namelen    = sizeof cliaddr;
  msg.msg_iov        = iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = &cmsg;
  msg.msg_controllen = sizeof cmsg;

  for (;;) {
    printf("recvmsg...");

    nbytes = recvmsg(sock, &msg, 0);
    if (nbytes == -1) {
      perror("recvfrom");
      exit(EXIT_FAILURE);
    }

    printf("%ld bytes ", (long)nbytes);
    printf("from %s:%hu ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    
    hexDump("pls",&msg,sizeof(msg));

    for (cmsgptr = CMSG_FIRSTHDR(&msg);
	 cmsgptr != NULL;
	 cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {

      if (cmsgptr->cmsg_level == IPPROTO_IP &&
	  cmsgptr->cmsg_type == DSTADDR_SOCKOPT) {

	printf("to %s", inet_ntoa(*(struct in_addr *)dstaddr(cmsgptr)));
      }
    }

    putchar('\n');
  }

  /*NOTREACHED*/
  return EXIT_SUCCESS;
}
