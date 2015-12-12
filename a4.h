#ifndef A4_HW_H
#define A4_HW_H

#include "unp.h"
#include "sockaddr_util.h"
#include "hw_addrs.h"

#define PROT_MAGIC 0x4d70
#define CLIENT_ADDR	"/home/cse533-7/client_path"
#define SUN_PATH	"/home/cse533-7/areq"
#define SUN_PATH2	"/home/cse533-7/areq2"
#define MAXIP 20
#define PROTOCOL 0xAD
#define IDENTIFICATION 0x9D
#define MCAST_IP	"224.0.0.18"
#define MCAST_PORT	2038
#define TRUE 1
#define FALSE 0
#define TIMEOUT 5	

struct hwaddr {
 int sll_ifindex; 				/* Interface number */
 unsigned short sll_hatype; 	/* Hardware type */
 unsigned char sll_halen; 		/* Length of address */
 unsigned char sll_addr[8]; 	/* Physical layer address */
};

struct areq_struct{
  struct hwaddr dest_hw;
  struct in_addr dest_ip;

};

typedef struct areq_struct areq_struct;

void findHostName(char *ip, char *host);
void findOwnIP(char *own_ip);

int areq(struct in_addr *IPaddr, socklen_t sockaddrlen, struct hwaddr *Hwaddr);

#endif
