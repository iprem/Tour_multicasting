#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "unp.h"
#include "a4.h"
#include "sockaddr_util.h"


#define CACHE_SIZE 20
#define ARP_REQUEST 1 
#define ARP_REPLY 2
#define ARP_PROTO 0x4d4d

struct arp_record{
	
	struct in_addr ip;
	unsigned char sll_addr[8]; 	/* Physical layer address */
	int sll_ifindex; 	  /* Interface number */
	unsigned short sll_hatype; 	/* Hardware type */
	
	int fd;
	int valid; /* -1 if invalid, 0 if incomplete, 1 if valid */

};

typedef struct arp_record arp_record;


struct arp{

	struct ethhdr hdr;
	unsigned char arp_prot[2];
	unsigned char hard_type[2];
	unsigned short prot_type;
	unsigned char hard_size;
	unsigned char prot_size;
	unsigned char op;
	unsigned char send_eth_addr[6];
	struct in_addr send_ip;
	unsigned char dest_eth_addr[6];
	struct in_addr dest_ip;
	
};

typedef struct arp arp;

void cache_init(arp_record * cache);
static inline void areq_get_args(int fd, areq_struct * areq);
int cache_have_hw(arp_record* cache, areq_struct * areq);
void areq_reply(int fd, areq_struct * areq, arp_record * cache);
void cache_insert_incomplete(int fd, arp_record * cache, areq_struct* areq);
void cache_add(arp_record* cache, arp* arp_recv);
void arp_send_request(int fd, areq_struct* areq);
void ethhdr_fill_src(struct ethhdr* hdr, int index);
void arp_send_request(int fd, areq_struct* areq);
static inline void arp_get_request(int fd, arp* recv, struct sockaddr_ll* sockaddr_request);
int arp_target(arp* incoming, struct in_addr* own_ip_network);
void arp_send_reply(int fd, arp* request, struct sockaddr_ll* sockaddr_request );

int main(int argc, char **argv){
	struct sockaddr_un sock_domain_arp;
	struct sockaddr_ll sock_packet_arp;
	struct in_addr own_ip_network;
	areq_struct areq_recv;
	arp arp_recv;
	fd_set rset;
	int max_fd = 0;
	int packet_fd = 0, domain_fd = 0;
	arp_record arp_cache[CACHE_SIZE];
	char own_ip[16];

	memset(&own_ip_network, 0 , sizeof(struct in_addr));
	memset(own_ip, 0 , 16);
	memset(&areq_recv, 0, sizeof(areq_struct));
	memset(&arp_recv, 0, sizeof(arp));
	cache_init(arp_cache);

	findOwnIP(own_ip);
	inet_pton(AF_INET, own_ip, &own_ip_network);

        /*Create packet socket*/
	packet_fd = Socket(AF_PACKET, SOCK_RAW, htons(PROT_MAGIC));

        /*Create domain socket */
	domain_fd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	init_sockaddr_un(&sock_domain_arp, SUN_PATH);
	Bind(domain_fd, (SA *) &sock_domain_arp, SUN_LEN(&sock_domain_arp));

	while(1){
		      /*Listen on sockets */
		      FD_ZERO(&rset);
		      FD_SET(packet_fd, &rset);
		      FD_SET(domain_fd, &rset);
		      max_fd = packet_fd > domain_fd ? packet_fd : domain_fd;
		      select(max_fd + 1, &rset, NULL, NULL, NULL);
		      if(FD_ISSET(packet_fd, &rset)){
			      memset(&areq_recv, 0 , sizeof(areq_struct));
			      arp_get_request(packet_fd, &arp_recv, &sock_packet_arp);
			      if(arp_target(&arp_recv, &own_ip_network)){
				      if(arp_recv.op == ARP_REPLY){
					      //Write to domain
					      cache_add(arp_cache, &arp_recv);					     
					      areq_reply(domain_fd, &areq_recv, arp_cache);
				      }

				      else if(arp_recv.op == ARP_REQUEST){
					      arp_send_reply(packet_fd, &arp_recv, &sock_packet_arp);
				      }
			      }
		      }
		      if(FD_ISSET(domain_fd, &rset)){
			      areq_get_args(domain_fd, &areq_recv);
			      if(cache_have_hw(arp_cache, &areq_recv)){
				      areq_reply(domain_fd, &areq_recv,  arp_cache);
			      }
			      else
			      {
				      cache_insert_incomplete(domain_fd, arp_cache, &areq_recv);
				      arp_send_request(packet_fd, &areq_recv);
			      }
		      }
	}
	
	
}

void 
ethhdr_fill_src(struct ethhdr* hdr, int index){
	struct hwa_info *hwa, *hwahead;
	
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next)
	{
		if (hwa->if_index == index)
		{
			memcpy(hdr->h_source, hwa->if_haddr, ETH_ALEN);
		}
	} 
	
}

void
arp_send_reply(int fd, arp* request, struct sockaddr_ll* sockaddr_request ){

	arp reply;
	struct sockaddr_ll sockaddr_reply;
	char own_ip[16];
	
	memset(&reply, 0 , sizeof(arp));
	memset(own_ip, 0 , 16);
	memset(&sockaddr_reply, 0, sizeof(struct sockaddr_ll));

	sockaddr_reply.sll_family = AF_PACKET;
	memcpy(sockaddr_reply.sll_addr, request->send_eth_addr , ETH_ALEN);
	sockaddr_reply.sll_ifindex = sockaddr_request->sll_ifindex;
	sockaddr_reply.sll_halen = ETH_ALEN;

	memcpy(reply.hdr.h_dest, request->send_eth_addr, ETH_ALEN);
	reply.hdr.h_proto = htons(PROT_MAGIC);
	ethhdr_fill_src(&reply.hdr, sockaddr_request->sll_ifindex);
	
	reply.prot_type = htons(ARP_PROTO);
	memcpy(reply.send_eth_addr, reply.hdr.h_source, 6);
	findOwnIP(own_ip);
	inet_pton(AF_INET, own_ip, &reply.send_ip);
	reply.op = ARP_REPLY;

	/* Make source addr the new target addr */
	memcpy(reply.dest_eth_addr, request->send_eth_addr, ETH_ALEN);
	memcpy(&reply.dest_ip, &request->dest_ip, ETH_ALEN);

	Sendto(fd, &request, sizeof(request), 0, (SA *) &sockaddr_reply, sizeof(struct sockaddr_ll));

}

void
arp_send_request(int fd, areq_struct* areq){

	arp request;
	struct sockaddr_ll sockaddr_request;
	unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	char own_ip[16];
		
	memset(&request, 0 , sizeof(arp));
	memset(own_ip, 0 , 16);
	memset(&sockaddr_request, 0, sizeof(struct sockaddr_ll));

	sockaddr_request.sll_family = AF_PACKET;
	memcpy(sockaddr_request.sll_addr, dest_mac, ETH_ALEN);
	sockaddr_request.sll_ifindex = areq->dest_hw.sll_ifindex;
	sockaddr_request.sll_halen = ETH_ALEN;

	memcpy(&request.hdr.h_dest, dest_mac, ETH_ALEN);
	request.hdr.h_proto = htons(PROT_MAGIC);
	ethhdr_fill_src(&request.hdr, areq->dest_hw.sll_ifindex);

	request.prot_type = htons(ARP_PROTO);
	memcpy(request.send_eth_addr, request.hdr.h_source, 6);
	findOwnIP(own_ip);
	inet_pton(AF_INET, own_ip, &request.send_ip);
	request.op = ARP_REQUEST;

	Sendto(fd, &request, sizeof(request), 0, (SA *) &sockaddr_request, sizeof(struct sockaddr_ll));
	
}

static inline void
arp_get_request(int fd, arp* recv, struct sockaddr_ll* sockaddr_request){
	
	Read(fd, recv, sizeof(arp));

}

int
arp_target(arp* incoming, struct in_addr* own_ip_network){
	if(!memcmp(&incoming->dest_ip, own_ip_network, sizeof(struct in_addr))){
		return TRUE;
	}
	else
		return FALSE;
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
cache_add(arp_record* cache, arp* arp_recv){
	int i;

	for(i = 0; i < CACHE_SIZE; i++){
		if(!memcmp(&cache->ip, &arp_recv->send_ip, sizeof(struct in_addr))){
			if(cache->valid == 0){
				memcpy(cache->sll_addr, arp_recv->send_eth_addr, ETH_ALEN);
				cache->valid = 1;
				break;
			}
		}
		cache++;
	}
}

void
cache_insert_incomplete(int fd, arp_record * cache, areq_struct* areq)
{
	int i;

	for(i = 0; i < CACHE_SIZE; i++){
		if(cache->valid == -1){
			memcpy(&cache->ip, &areq->dest_ip, sizeof(struct in_addr));
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
	int i;

	for(i = 0; i < CACHE_SIZE; i++){
		if(!memcmp(&areq->dest_ip, &cache->ip, sizeof(struct in_addr))){
			memcpy(areq_reply->dest_hw.sll_addr, cache->sll_addr, 8);
			areq_reply->dest_hw.sll_ifindex = cache->sll_ifindex; 
			areq_reply->dest_hw.sll_halen = 6;
			memcpy(&areq_reply->dest_ip, &cache->ip, sizeof(struct in_addr));
			break;
		}
		cache++;
	}

	return;
}

int
cache_have_hw(arp_record* cache, areq_struct * areq){
	int i;
	
	for(i = 0; i < CACHE_SIZE; i++){
		if(!memcmp(&areq->dest_ip, &cache->ip, sizeof(struct in_addr))){
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
	
	cache_get_info(areq, &areq_reply,  cache);
	
	Write(fd, &areq_reply, sizeof(areq_struct));
}



