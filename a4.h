
#include "unp.h"
#include "sockaddr_util.h"
#include "hw_addrs.h"


#define PROT_MAGIC 0x4d70
#define SUN_PATH	"/home/cse533-7/areq"
#define MAXIP 20
#define PROTOCOL 0xAD
#define IDENTIFICATION 0x9D
#define MCAST_IP	"224.0.0.18"
#define MCAST_PORT	2038


struct hwaddr {
 int sll_ifindex; 				/* Interface number */
 unsigned short sll_hatype; 	/* Hardware type */
 unsigned char sll_halen; 		/* Length of address */
 unsigned char sll_addr[8]; 	/* Physical layer address */
};

void findHostName(char *ip, char *host);
void findOwnIP(char *own_ip);
int areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *Hwaddr);