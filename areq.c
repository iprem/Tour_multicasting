#include "unp.h"
#include "a4.h"
#include "sockaddr_util.h"

int 
areq (struct in_addr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){

	int sockfd;
	const int one = 1;
	struct sockaddr_un addr;
	areq_struct ar;
	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	int n;
	
	sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	init_sockaddr_un(&addr, SUN_PATH);
	
	Connect(sockfd, (SA *) &addr, sizeof(addr));
	
	memset(&ar, 0, sizeof(struct areq_struct));

	ar.dest_hw = *HWaddr;
	ar.dest_ip = *IPaddr;

	printf("Requesting HW address of%s", inet_ntoa(ar.dest_ip));

	/* Set timeout of 5 secs for the socket */	
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		err_quit("setsockopt failed\n");
	
	Send(sockfd, &ar, sizeof(ar), 0);

	n = recv(sockfd, HWaddr, sizeof(HWaddr), 0 );
	if((n < 0) || (n == EWOULDBLOCK))
		return -1;
	
	close(sockfd);
	return n; 
		
}
