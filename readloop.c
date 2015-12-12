#include	"ping.h"
#include <linux/if_ether.h> 

void
readloop(struct proto *pr)
{
	int				size;

	sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	setuid(getuid());		/* don't need special permissions any more */

	size = 60 * 1024;		/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	

	for ( ; ; ) {
		
		/* Send packets every second */
		send_v4(sockfd, pr);
			
		sleep(1);

	}
}
