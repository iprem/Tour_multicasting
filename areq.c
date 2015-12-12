#include "unp.h"
#include "a4.h"
#include "sockaddr_util.h"

int 
areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){

	int sockfd;
	const int one = 1;
	struct sockaddr_un addr;
	
	sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	init_sockaddr_un(&addr, SUN_PATH);

	Bind(sockfd, (SA *)&addr, sizeof(addr));
	
	

}
