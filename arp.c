#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "unp.h"
#include "a4.h"
#include "sockaddr_util.h"


#define CACHE_SIZE 20

struct arp_record{
	
	struct in_addr ip;
	unsigned char sll_addr[8]; 	/* Physical layer address */
	int sll_ifindex; 	  /* Interface number */
	unsigned short sll_hatype; 	/* Hardware type */
	
	int fd;
	int valid; /* -1 if invalid, 0 if incomplete, 1 if valid */

};

typedef struct arp_record arp_record;

void init_cache(arp_record *cache);

int main(int argc, char **argv){
	struct sockaddr_un sock_domain_arp;
	areq_struct areq_recv;
	
	fd_set rset;
	int max_fd = 0;
	int packet_fd = 0, domain_fd = 0;
	char *domain_path = "arp_path";
	arp_record arp_cache[CACHE_SIZE];

	memset(&areq, 0, sizeof(areq_struct));
	cache_init(arp_cache);

        /*Create packet socket*/
	packet_fd = Socket(AF_PACKET, SOCK_RAW, htons(PROT_MAGIC));

        /*Create domain socket */
	domain_fd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	init_sockaddr_un(&sock_domain_arp, domain_path);
	Bind(domain_fd, (SA *) &sock_domain_arp, SUN_LEN(&sock_domain_arp));

	while(1){
		      /*Listen on sockets */
		      FD_ZERO(&rset);
		      FD_SET(packet_fd, &rset);
		      FD_SET(domain_fd, &rset);
		      max_fd = packet_fd > domain_fd ? packet_fd : domain_fd;
		      select(max_fd + 1, &rset, NULL, NULL, NULL);
		      if(FD_ISSET(packet_fd, &rset)){

		      }
		      if(FD_ISSET(domain_fd, &rset)){
			      areq_get_args(domain_fd, &areq_recv);
			      if(cache_have_hw(arp_cache)){
				      areq_reply(domain_fd, &areq_recv,  arp_cache);
			      }
			      else
			      {
				      cache_insert_incomplete(domain_fd, arp_cache, &areq_recv);
				      arp_request(packet_fd, &areq_recv);
			      }
			      
			      
		      }
	}
	
	
}





/*

Zero cache and set each cache record fd to -1

@cache pointer to first cache entry

*/

void
cache_init(arp_record * cache){
	int i;
	
	memset(cache, 0, sizeof(arp_record) * CACHE_SIZE);
	
	for(i = 0; i < CACHE_SIZE; i++){
		cache->valid = -1;
		cache++;
	}
	
	return;
}

void
cache_insert_incomplete(int fd, arp_record * cache, areq_struct* areq)
{
	for(i = 0; i < CACHE_SIZE; i++){
		if(cache->valid == -1){
			memcpy(cache->ip, areq->dest_ip, sizeof(in_addr));
			cache->sll_ifindex = areq->dest_hw.sll_ifindex;
			cache->fd = fd;
			cache->sll_hatype = areq->dest_hw.sll_hatype;
			cache->valid = 0;
		}
		cache++;
	}
}

/*

Get eth addr, index and IP addr from cache. Set addr len to eth addr len. 

*/

void
cache_get_info(areq_struct * areq, areq_struct * areq_reply, arp_record * cache ){
	for(i = 0; i < CACHE_SIZE; i++){
		if(!memcmp(areq->dest_ip, cache->ip, sizeof(struct in_addr))){
			memcpy(areq_reply->dest_hw.sll_addr, cache->sll_addr, 8);
			areq_reply->dest_hw.sll_ifindex = cache->sll_ifindex; 
			areq_reply->dest_hw.sll_halen = 6;
			memcpy(areq_reply->dest_ip, cache->ip, sizeof(struct in_addr));
			break;
		}
		cache++;
	}

	return;
}

int
cache_have_hw(arp_record* cache, areq_struct * areq){
	
	for(i = 0; i < CACHE_SIZE; i++){
		if(!memcmp(areq->dest_ip, cache->ip, sizeof(struct in_addr))){
			if(cache->valid ==  1)
				return TRUE;
		}
		cache++;
	}
	
	return FALSE;
}



static inline void
areq_get_args(int fd, areq_struct * areq){
	
	Read(fd, areq, sizeof(areq_struct));

}

void
areq_reply(int fd, areq_struct * areq, arp_record * cache){
	areq_struct areq_reply;
	
	memset(&areq_reply, 0, sizeof(areq_struct));
	
	cache_get_info(areq, areq_reply,  cache);
	
	Write(fd, areq_reply, sizeof(areq_struct));
}



