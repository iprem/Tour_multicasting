#include "unp.h"
#include "a4.h"
#include "sockaddr_util.h"



int 
areq (struct in_addr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){
  
	socklen_t len = sizeof(struct sockaddr_un);
	int sockfd;
	const int one = 1;
	struct sockaddr_un addr;
	struct sockaddr_un myaddr;
	areq_struct ar;
	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	int n, i;
	
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	init_sockaddr_un(&addr, SUN_PATH2);
	init_sockaddr_un(&myaddr, CLIENT_ADDR);
	unlink(CLIENT_ADDR);
	Bind(sockfd, (SA*) &myaddr, sizeof(myaddr));
	
	memset(&ar, 0, sizeof(struct areq_struct));

	//ar.dest_hw = HWaddr;
	ar.dest_ip = *IPaddr;
	
	memcpy(&ar.dest_hw, HWaddr, sizeof(struct hwaddr));

	/*HWaddr->sll_addr[0]= 0x00;
	HWaddr->sll_addr[1]= 0x0c;
	HWaddr->sll_addr[2]= 0x29;
	HWaddr->sll_addr[3]= 0x49;
	HWaddr->sll_addr[4]= 0x3f;
	HWaddr->sll_addr[5]= 0x5b;*/

	printf("Requesting HW address of %s \t\n", inet_ntoa(ar.dest_ip));
	
	/*for (i=0; i<5; i++) {
    	printf ("%02x:", HWaddr->sll_addr[i]);
  	}
  	printf ("%02x\n", HWaddr->sll_addr[5]);*/

	/* Set timeout of 5 secs for the socket */	
	/*if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		err_quit("setsockopt failed\n");*/
	
	Sendto(sockfd, &ar, sizeof(ar), 0, (SA *)&addr, sizeof(addr));

	n = recvfrom(sockfd, &ar, sizeof(ar), 0, (SA *)&addr, &len);
	memcpy(HWaddr->sll_addr, ar.dest_hw.sll_addr, ETH_ALEN);
	if((n < 0) || (n == EWOULDBLOCK))
		return -1;
	
	close(sockfd);
	return sizeof(HWaddr); 
		
}
