#include	"unp.h"
#include	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>

#define	BUFSIZE		1500
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define ICMP_HDRLEN 8  // ICMP header length for echo request, excludes data

			/* globals */
char	 sendbuf[BUFSIZE];

int		 datalen;			/* # bytes of data following ICMP header */
pid_t	 pid;				/* our PID */
int		 sockfd, pg;

struct proto {
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */
  socklen_t	    salen;		/* length of sockaddr{}s */
  int	   	    icmpproto;	/* IPPROTO_xxx value for ICMP */
  char 		host[20];
  int		 nsent;			/* add 1 for each sendto() */
  int			i;
};

			/* function prototypes */
void	 proc_v4(char *, ssize_t, struct msghdr *, struct timeval *);
void	 send_v4(int, struct proto *);
void	 readloop(struct proto *);
void	 sig_alrm(int);
void	 tv_sub(struct timeval *, struct timeval *);

