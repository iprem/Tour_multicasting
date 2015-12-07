/* Tour program */


#include "unp.h"
#include <netinet/ip.h>
#include <net/if.h> 
#include "ping.h"
#include "a4.h"
#include "unpthread.h"
#include <time.h>

#define IP4_HDRLEN 20         // IPv4 header length
#define MAXIP 20
#define PROTOCOL 0xAD
#define IDENTIFICATION 0x9D
#define MCAST_IP	"224.0.0.18"
#define MCAST_PORT	2038



struct proto proto_v4 =
		{ proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP };

struct threads{

	pthread_t tid;
	struct in_addr ip;
	
	struct threads *next;

};

static struct threads *threads;

struct proto_hdr{

	struct in_addr src_ip;			//Source IP
	struct in_addr dst_ip;			//Destination IP
	uint16_t ptr;					//Pointer for maintaining next node
	uint16_t max;					//Total number of nodes in tour	

};

struct payload{

	struct proto_hdr proto_hdr;	
	struct in_addr nodes[MAXIP];	//Nodes in the tour		
	struct sockaddr_in mcastaddr;	//Multicast address

};

int     datalen = 56;

void iphdr_init (struct ip *iphdr);					/* To initialize IP header */
uint16_t checksum (uint16_t *addr, int len); 		/* Find checksum */
int send_packet (int rt, struct payload *payload);	/*Send packets*/
void run_tour (int rt, int udp_send, int udp_recv, int binded);	/* Run the tour */
void * ping_t (void *);
int add_node (struct threads *);
void kill_threads ();
void *test(void *arg);

int main(int argc , char ** argv){
	
	int rt, udp_send, udp_recv;			/* Sockets */
	struct payload *payload;
	int i = 1, binded = 0;
	char interface[4], own_ip[16], hostname[4];
	struct ifreq ifr;
	struct hostent *he;
	struct in_addr **addr_list;


	if(argc > MAXIP)
		err_quit("Total number of nodes in the tour cannot exceed %d\n", MAXIP);
	
	payload = (struct payload *)  malloc(sizeof(struct payload));

	/* Create rt socket for tour */
	rt = Socket(AF_INET, SOCK_RAW, PROTOCOL);

	/* Create socket to send multicast messages */
	udp_send = Socket(AF_INET, SOCK_DGRAM, 0);

	/*Create socket to receive multicast messages */
	udp_recv = Socket(AF_INET, SOCK_DGRAM, 0);

	/* Find IP of host */
	findOwnIP(own_ip);
	strcpy (interface, "eth0");


	// Submit request for a socket descriptor to look up interface.
	if ((rt = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror ("socket() failed to get socket descriptor for using ioctl() ");
		exit (EXIT_FAILURE);
	}

	// Use ioctl() to look up interface index which we will use to
	// bind socket descriptor sd to specified interface with setsockopt() since
	// none of the other arguments of sendto() specify which interface to use.
	memset (&ifr, 0, sizeof (ifr));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	if (ioctl (rt, SIOCGIFINDEX, &ifr) < 0) {
		perror ("ioctl() failed to find interface ");
		return (EXIT_FAILURE);
	}
	close (rt);
	//printf ("Index for interface %s is %i\n", interface, ifr.ifr_ifindex);
	
	// Submit request for a raw socket descriptor.
	if ((rt = socket (AF_INET, SOCK_RAW, PROTOCOL)) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	// Bind socket to interface index.
	if (setsockopt (rt, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr)) < 0) {
		perror ("setsockopt() failed to bind to interface ");
		exit (EXIT_FAILURE);
	}

	findHostName(own_ip,hostname);	

	if(argc > 1){

		if( !strcmp(hostname, argv[1]) )
			err_quit("Tour cannot start with source node itself\n");
		
		/* Copy source address to first place */
		Inet_pton(AF_INET, own_ip, &(payload->nodes[0]));

		while(i < argc){

			/* Get IP address of nodes in tour */
			if ((he = gethostbyname(argv[i])) == NULL )
				err_quit("Error: Gethostbyname for node %s\nPlease use correct hostnames\n", argv[i]);

			addr_list = (struct in_addr **)he->h_addr_list;
			payload->nodes[i] = *addr_list[0] ;

			printf("Addr: %s\n",inet_ntoa(payload->nodes[i]));
			
			if((i+1) != argc)
				if( !strcmp(argv[i],argv[i+1]) )
					err_quit("Two consecutive nodes on the tour cannot be same\n");
			i++;
		}

		/* Pointer should point to next hop */
		payload->proto_hdr.ptr = htons(0);
		payload->proto_hdr.max = htons(i);
		printf("Payload success\n");
		
		memset (&(payload->mcastaddr), 0, sizeof (struct sockaddr_in));
		payload->mcastaddr.sin_family = AF_INET;
		Inet_pton(AF_INET, MCAST_IP, &(payload->mcastaddr.sin_addr));
		payload->mcastaddr.sin_port = htons(MCAST_PORT);

		/* Bind to multicast port */
		Bind(udp_recv, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr));
		
		/* Connect to multicast address */
		Connect(udp_send, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr) );
		
		/* Join multicast group on arbitarary interface */
		Mcast_join(udp_recv, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr),0,0);
		send_packet(rt, payload);
		binded = 1;
	
	}

	run_tour(rt, udp_send, udp_recv, binded);	
	
	
	close(rt);
	close(udp_send);
	close(udp_recv);
		
	exit(0);
}

void run_tour(int rt, int udp_send, int udp_recv, int binded){
	
	struct payload *payload;
	struct ip *iphdr;
	uint8_t *buff;	
	int flag = 0, flag2 = 0, n = 5;
	time_t ticks;
	ticks = time(NULL);
	socklen_t addrlen;
	struct sockaddr_in source;
	char hostname[4], hostname_src[4], own_ip[16];
	char msg[MAXLINE];
	pthread_t tid;
	struct threads *newnode;

	threads = (struct threads *)malloc(sizeof(struct threads));
	threads = NULL;

	struct timeval tv;
	tv.tv_sec = 1000;

	buff = (uint8_t *) malloc(IP_MAXPACKET * sizeof (uint8_t));
	payload = (struct payload *)  malloc(sizeof(struct payload));

	fd_set rset;
	int maxfd;

	sigset_t mask;
	sigset_t orig_mask;

	maxfd = rt > udp_recv ? rt+1 : udp_recv+1 ;

	sigemptyset (&mask);
	//sigaddset (&mask, SIGINT);
	//sigaddset (&mask, SIGCONT);
	sigaddset (&mask, SIGALRM);
	sigaddset (&mask, SIGILL);
	
	while(1){
	
		FD_ZERO(&rset);
		FD_SET(rt, &rset);
		FD_SET(udp_recv, &rset);

		if( select(maxfd, &rset, NULL, NULL, &tv) < 0 ){
			//printf("Select failed: Error %d\n", errno );
			continue;
		}
				
		if(FD_ISSET(rt, &rset)){
			if(recvfrom(rt, buff, (IP4_HDRLEN + sizeof(struct payload)), 0, (SA *)&source, &addrlen) < 0)
			//printf("Error\n");
			memset(payload, 0, sizeof(payload));
			iphdr = (struct ip *) buff;
	
			/* Discard the packet if identification field is not same */
			if(iphdr->ip_id != htons(IDENTIFICATION))
				continue;

			payload = (struct payload *)(buff + IP4_HDRLEN);
			findHostName((inet_ntoa(iphdr->ip_dst)), hostname);
			findHostName((inet_ntoa(iphdr->ip_src)), hostname_src);
			//printf("Own hostname: %s\tMCAST_PORT: %d\n",hostname,ntohs(payload->mcastaddr.sin_port));
			printf("\n<%.24s> received source routing packet from %s\n", ctime(&ticks), hostname_src);
			
			/*
				Start Pinging
			*/
			
				/*Create thread to start pinging*/
			Pthread_create(&tid, NULL, ping_t, hostname_src);	
			newnode = (struct threads *)malloc (sizeof (struct threads));		
			newnode->tid = tid;
			newnode->ip = iphdr->ip_src;
			newnode->next = NULL;
			add_node(newnode);

			if(send_packet(rt, payload)){

				sprintf(msg,"<<<<< This is node %s . Tour has ended . Group members please identify yourselves. >>>>>", hostname);
				printf("\nNode %s . Sending <%s>\n", hostname, msg);

				while (n != 0){
					n = sleep(n);
				}
				kill_threads();
				
				Sendto(udp_send, msg, sizeof(msg), 0, (SA *)&payload->mcastaddr, sizeof(payload->mcastaddr));
				flag2 = 1;
			}				
			
			/* Join multicast group */
			if(!binded){
				/* Bind to multicast port */
				Bind(udp_recv, (SA *)&(payload->mcastaddr), addrlen);
		
				/* Connect to multicast address */
				Connect(udp_send, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr) );
				
				Mcast_join(udp_recv, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr),0,0);

				binded = 1;
				//printf("\nJoined Multicast Group\n");
			}
		}
		
		if(FD_ISSET(udp_recv, &rset)){
			memset(&source, 0, sizeof(struct sockaddr));
			memset(msg, 0, MAXLINE);
			findOwnIP(own_ip);
			findHostName(own_ip, hostname);
			
			if(!flag2){
				sleep(5);
				kill_threads();
			}

			Recvfrom(udp_recv, msg, sizeof(msg), 0, (SA *)&source, &addrlen);
			printf("\nNode %s . Received: %s\n", hostname, msg);
			
			
			if(!flag){
				memset(msg, 0, MAXLINE);
				sprintf(msg, "<<<<< Node %s . I am a member of the group. >>>>>", hostname);
				printf("\nNode %s . Sending: %s\n", hostname, msg);
				Send(udp_send, msg, sizeof(msg), 0);
				flag = 1;
				tv.tv_sec = 3;
			}

		}

		if((int)tv.tv_sec == 0){
			printf("\nThank you for joining multicast session. Ending session now\n\n");
			sleep(2);
			break;
		}
	}

	

}


int send_packet(int rt, struct payload *payload){
	
	struct ip iphdr;
	uint8_t *buff;
	char src_ip[16], own_ip[16];
	struct sockaddr_in sin;
	socklen_t addrlen;
	const int on = 1;
	int temp;

	iphdr_init(&iphdr); //Fill in header

	temp = htons(payload->proto_hdr.ptr);
	temp++;

	if((temp+1) > htons(payload->proto_hdr.max))
		return 1;

	payload->proto_hdr.ptr = htons(temp);

	buff = (uint8_t *) malloc(IP_MAXPACKET * sizeof (uint8_t));

	findOwnIP(own_ip);
	strcpy(src_ip, own_ip);

	/* IP Header */

	// Fill source and destination address in header of IP and our protocol
	iphdr.ip_src = payload->nodes[temp-1];
	Inet_pton(AF_INET, src_ip, &(payload->proto_hdr.src_ip));
	iphdr.ip_dst = payload->nodes[temp];
	payload->proto_hdr.dst_ip = payload->nodes[temp];
	
	// Add packet to buffer
	memcpy (buff, &iphdr, IP4_HDRLEN * sizeof (uint8_t));
	memcpy((buff+IP4_HDRLEN), payload, sizeof(struct payload));

	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = iphdr.ip_dst.s_addr;
	addrlen = sizeof(sin);

	// Set flag so socket expects us to provide IPv4 header.
	if (setsockopt(rt, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
    	err_quit("Error: Couldn't set IP_HDRINCL option for rt socket\n");

	// Send packet.
	if ((sendto (rt, buff, (IP4_HDRLEN + sizeof(struct payload)) , 0, (SA *) &sin, addrlen)) < 0)  {
		perror ("Error: sendto() failed\n ");
		exit (EXIT_FAILURE);
	}
	
	return 0;

}

void iphdr_init(struct ip *iphdr){

	int ip_flags[4];
	//int datalen = sizeof(struct packet);

	// IPv4 header length (4 bits): Number of 32-bit words in header = 5
	iphdr->ip_hl = IP4_HDRLEN / sizeof (uint32_t);

	// Internet Protocol version (4 bits): IPv4
	iphdr->ip_v = (4);

	// Type of service (8 bits)
	iphdr->ip_tos = (0);

	 // Total length of datagram (16 bits): IP header + payloadlen
	iphdr->ip_len = htons (IP4_HDRLEN + 4*sizeof(uint8_t));

	// ID identification (16 bits)
	iphdr->ip_id = htons (IDENTIFICATION);

	// Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

	// Zero (1 bit)
	ip_flags[0] = 0;

	// Do not fragment flag (1 bit)
	ip_flags[1] = 0;

	// More fragments following flag (1 bit)
	ip_flags[2] = 0;

	// Fragmentation offset (13 bits)
	ip_flags[3] = 0;

	iphdr->ip_off = htons ((ip_flags[0] << 15)
						+ (ip_flags[1] << 14)
						+ (ip_flags[2] << 13)
							+  ip_flags[3]);

	// Time-to-Live (8 bits): default to maximum value
	iphdr->ip_ttl = 1;

	// Transport layer protocol (8 bits)
	iphdr->ip_p = PROTOCOL;

	// IPv4 header checksum (16 bits): set to 0 when calculating checksum
  	iphdr->ip_sum = 0;
  	iphdr->ip_sum = htons(checksum ((uint16_t *) iphdr, IP4_HDRLEN));

}


void * ping_t (void * arg){

	struct addrinfo *ai;
	char   *h;
	char *host = (char *) arg;

	//Pthread_detach(pthread_self());

	pid = getpid() & 0xffff;
	Signal(SIGALRM, sig_alrm);

	ai = Host_serv (host, NULL, 0, 0);

	h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
	printf ("PING %s (%s): %d data bytes\n",
			ai->ai_canonname ? ai->ai_canonname : h, h, datalen);
	
	pr = &proto_v4;
	pr->sasend = ai->ai_addr;
	pr->sarecv = Calloc (1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;
	
	readloop();
	
	return 0;

}


int add_node (struct threads *newnode){

	struct threads *cur = threads;
	
	if(cur == NULL){
		threads = newnode;
		printf("Added pid %d\n",getpid());
		return 1;
	}

	while (cur != NULL){
		if(!strcmp((inet_ntoa(cur->ip)),(inet_ntoa(newnode->ip))))
			return 0;
		cur = cur->next;
	}

	cur->next = newnode;

	return 1;

}


void kill_threads(){

	struct threads *cur = threads;
	
	while (cur != NULL){
		printf("Killing %d\n",getpid());
		pthread_cancel(cur->tid);
		pthread_join(cur->tid, NULL);
		cur = cur->next;
	}
	
	free(threads);
	threads = NULL;

}

// Computing the internet checksum (RFC 1071)
uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}


void *test(void *arg){
	
	while(1){
		printf("Process ID: %d\n",getpid());
		sleep(2);
	}

}