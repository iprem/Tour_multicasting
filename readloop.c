#include	"ping.h"

void
readloop(struct proto *pr)
{
	int				size;
	char			recvbuf[BUFSIZE];
	char			controlbuf[BUFSIZE];
	struct msghdr	msg;
	struct iovec	iov;
	ssize_t			n;
	struct timeval	tval;
	
	//printf("Printf: ICMP %d", pr->icmpproto);

	sockfd = Socket(AF_INET, SOCK_RAW, pr->icmpproto);
	//sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	setuid(getuid());		/* don't need special permissions any more */

	size = 60 * 1024;		/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	
	iov.iov_base = recvbuf;
	iov.iov_len = sizeof(recvbuf);
	msg.msg_name = pr->sarecv;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = controlbuf;
	for ( ; ; ) {
		msg.msg_namelen = pr->salen;
		msg.msg_controllen = sizeof(controlbuf);

		//printf("Send\n");
		send_v4(sockfd, pr);	
		//printf("Receive message\n");
		n = recvmsg(pg, &msg, 0);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else
				err_sys("recvmsg error");
		}

		Gettimeofday(&tval, NULL);

		proc_v4(recvbuf, n, &msg, &tval);
		
		sleep(1);
	}
}
