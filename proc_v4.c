#include	"ping.h"
#include "a4.h"

void
proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{
	int				hlen1, icmplen;
	double			rtt;
	struct ip		*ip;
	struct icmp		*icmp;
	struct timeval	*tvsend;
	char host[20];

	ip = (struct ip *) ptr;		/* start of IP header */
	hlen1 = ip->ip_hl << 2;		/* length of IP header */
	if (ip->ip_p != IPPROTO_ICMP){
		//return;				/* not ICMP */
	}

	findHostName(inet_ntoa(ip->ip_src), host);

	icmp = (struct icmp *) (ptr + hlen1);	/* start of ICMP header */
	if ( (icmplen = len - hlen1) < 8){
		//printf("Malformed packet from %s\n", host);
	//	return;				/* malformed packet */
	}

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid){
			printf("PID is not same\n");
			//return;			/* not a response to our ECHO_REQUEST */
		}
		if (icmplen < 16){
			//printf("Length not enough\n");
			//return;			/* not enough data to use */
		}

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
				icmplen, host,
				icmp->icmp_seq, ip->ip_ttl, rtt);

	} 
}
