
#include "unp.h"
#include "sockaddr_util.h"
#include "hw_addrs.h"


#define PROT_MAGIC 0x4d70


struct hwaddr {
 int sll_ifindex; 				/* Interface number */
 unsigned short sll_hatype; 	/* Hardware type */
 unsigned char sll_halen; 		/* Length of address */
 unsigned char sll_addr[8]; 	/* Physical layer address */
};

void findHostName(char *ip, char *host);
void findOwnIP(char *own_ip);
