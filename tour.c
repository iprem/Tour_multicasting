/* Tour program */


#include "unp.h"
#include <netinet/ip.h>
#include <net/if.h> 
#include "ping.h"
#include "a4.h"
#include "unpthread.h"
#include <time.h>



struct proto pr[10];

struct argt{
	struct proto p;
	char hostname[20];
}argt;

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

}*p;

void iphdr_init (struct ip *iphdr);					/* To initialize IP header */
uint16_t checksum (uint16_t *addr, int len); 		/* Find checksum */
int send_packet (int rt, struct payload *payload);	/*Send packets*/
void run_tour (int rt, int udp_send, int udp_recv, int pg, int binded);	/* Run the tour */
void * ping_t (void *);
int add_node (struct threads *);
void kill_threads (pthread_t *, int);
void *test(void *arg);
int check_existing_thread(char * );
void sig_alrm(int signo);

int datalen = 56, udp_send, i;
char msg1[MAXLINE];
pthread_t tid[10];

int main(int argc , char ** argv){
	
	int rt, udp_recv;			/* Sockets */
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

	/* Create pg socket for receiving pings */
	pg = Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

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

			//printf("Addr: %s\n",inet_ntoa(payload->nodes[i]));
			
			if((i+1) != argc)
				if( !strcmp(argv[i],argv[i+1]) )
					err_quit("Two consecutive nodes on the tour cannot be same\n");
			i++;
		}

		/* Pointer should point to next hop */
		payload->proto_hdr.ptr = htons(0);
		payload->proto_hdr.max = htons(i);
		//printf("Payload success\n");
		
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

	run_tour(rt, udp_send, udp_recv, pg, binded);	
	
	
	close(rt);
	close(udp_send);
	close(udp_recv);
		
	exit(0);
}

void run_tour(int rt, int udp_send, int udp_recv, int pg, int binded){
	
	struct payload *payload;
	struct ip *iphdr;
	uint8_t *buff;	
	int flag = 0, k = 0, flag2 = 0, flag3 = 0, flag4 = 0;
	time_t ticks;
	ticks = time(NULL);
	socklen_t addrlen;
	struct sockaddr_in source;
	char hostname[4], hostname_src[4], own_ip[16];
	char msg[MAXLINE];
	struct threads *newnode;
	int				size;
	char			recvbuf[BUFSIZE];
	char			controlbuf[BUFSIZE], *h;
	struct msghdr	mseg;
	struct iovec	iov;
	ssize_t			num;
	struct timeval	tval;
	struct sockaddr *sarecv;
	struct addrinfo *a;
	//struct proto p[10];

	sarecv = Calloc (1, sizeof(SA));
	pid = getpid() & 0xffff;

	size = 60 * 1024;		/* OK if setsockopt fails */
	setsockopt(pg, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	setsockopt(udp_recv, SOL_SOCKET, SO_RCVBUF, &size, 2*sizeof(size));
 
	iov.iov_base = recvbuf;
	iov.iov_len = sizeof(recvbuf);
	mseg.msg_name = sarecv;
	mseg.msg_iov = &iov;
	mseg.msg_iovlen = 1;
	mseg.msg_control = controlbuf;

	threads = (struct threads *)malloc(sizeof(struct threads));
	threads = NULL;

	struct timeval tv;
	tv.tv_sec = 100;

	buff = (uint8_t *) malloc(IP_MAXPACKET * sizeof (uint8_t));
	payload = (struct payload *)  malloc(sizeof(struct payload));

	fd_set rset;
	int maxfd;

	maxfd = rt > udp_recv ? rt : udp_recv ;
	maxfd = pg > maxfd ? pg+1 : maxfd +1 ;

	Signal(SIGALRM, sig_alrm);
	
	while(1){
	
		FD_ZERO(&rset);
		FD_SET(rt, &rset);
		FD_SET(udp_recv, &rset);
		FD_SET(pg, &rset);

		if( select(maxfd, &rset, NULL, NULL, &tv) < 0 ){
			
			if(errno != EINTR)
				printf("Select failed: Error %d\n", errno );
			continue;
		}
				
		if(FD_ISSET(rt, &rset)){
			if(recvfrom(rt, buff, (IP4_HDRLEN + sizeof(struct payload)), 0, (SA *)&source, &addrlen) < 0)
				printf("Error %d\n",errno);
			memset(payload, 0, sizeof(payload));
			iphdr = (struct ip *) buff;
	
			/* Discard the packet if identification field is not same */
			if(iphdr->ip_id != htons(IDENTIFICATION))
				continue;

			payload = (struct payload *)(buff + IP4_HDRLEN);
			findHostName((inet_ntoa(iphdr->ip_dst)), hostname);
			findHostName((inet_ntoa(iphdr->ip_src)), hostname_src);
			//printf("Own hostname: %s\tMCAST_PORT: %d\n",hostname,ntohs(payload->mcastaddr.sin_port));
			printf("\n<%.24s> received source routing packet from %s(%s)\n", ctime(&ticks), hostname_src, inet_ntoa(iphdr->ip_src));
			
			/*
				Start Pinging
			*/
			
			/*Create thread to start pinging*/
			if (check_existing_thread(inet_ntoa(iphdr->ip_src))){

				a = Host_serv (hostname_src, NULL, 0, 0);
				h = Sock_ntop_host(a->ai_addr, a->ai_addrlen);
				printf ("PING %s (%s): %d data bytes\n",
							a->ai_canonname ? a->ai_canonname : h, h, datalen);

				//pr[k].sasend = a->ai_addr;
				pr[k].sasend = iphdr->ip_src;
				pr[k].sarecv = iphdr->ip_dst;
				pr[k].salen = a->ai_addrlen;
				pr[k].icmpproto = IPPROTO_ICMP;
				strcpy(pr[k].host, hostname_src);
				pr[k].nsent = 0;
				pr[k].i = k;

				Pthread_create(&tid[i], NULL, ping_t, (void *)k);	
				//Pthread_create(&tid[i], NULL, test, (void *)k);	
				newnode = (struct threads *)malloc (sizeof (struct threads));		
				newnode->tid = tid[i];
				newnode->ip = iphdr->ip_src;
				newnode->next = NULL;
				add_node(newnode);
				k++;i++;
			}
			else
				printf("%s is already pinged\n", hostname_src);

			if(send_packet(rt, payload)){
				
				//printf("Setting memory\n");
				memcpy(&p, &payload, sizeof(payload));
				
				// Set alarm to allow 5 mins pinging
				alarm(5);

				sprintf(msg1,"<<<<< This is node %s . Tour has ended . Group members please identify yourselves. >>>>>", hostname);

				flag3 = 1;

			}				
			
			/* Join multicast group */
			if(!binded){
				/* Bind to multicast port */
				Bind(udp_recv, (SA *)&(payload->mcastaddr), addrlen);
		
				/* Connect to multicast address */
				Connect(udp_send, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr) );
				
				Mcast_join(udp_recv, (SA *)&(payload->mcastaddr), sizeof(payload->mcastaddr),0,0);

				binded = 1;
			}
		}
		if(!flag4){
		if(FD_ISSET(pg, &rset)){

			mseg.msg_namelen = sizeof(SA);
			mseg.msg_controllen = sizeof(controlbuf);
			num = recvmsg(pg, &mseg, 0);
			if (num < 0) {
				if (errno == EINTR)
					continue;
				else
					err_sys("recvmsg error");
			}

			Gettimeofday(&tval, NULL);
			proc_v4(recvbuf, num, &mseg, &tval);
		}
		}
		
		if(FD_ISSET(udp_recv, &rset)){
			memset(&source, 0, sizeof(struct sockaddr));
			memset(msg, 0, MAXLINE);
			findOwnIP(own_ip);
			findHostName(own_ip, hostname);
						
			Recvfrom(udp_recv, msg, sizeof(msg), 0, (SA *)&source, &addrlen);
			
			if(flag3){
				printf("\nNode %s . Sending <%s>\n", hostname, msg1);
				flag3 = 0;
			}

			if(!flag2){
				kill_threads(tid, i-1);
				flag2 = 1;
			}

			printf("Node %s . Received: %s\n", hostname, msg);			

			if(!flag){
				memset(msg, 0, MAXLINE);
				sprintf(msg, "<<<<< Node %s . I am a member of the group. >>>>>", hostname);
				printf("\nNode %s . Sending:  %s\n", hostname, msg);
				Send(udp_send, msg, sizeof(msg), 0);
				flag = 1;
				tv.tv_sec = 3;
			}
			flag4 = 1;

		}

		if((int)tv.tv_sec == 0){
			printf("\nThank you for joining multicast session. Ending session now\n\n");
			sleep(1);
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

	int size;
		
	size = (int *)arg;

	readloop(&pr[size]);
	
	return 0;

}


int add_node (struct threads *newnode){

	struct threads *cur = threads;

	if(cur == NULL){
		threads = newnode;	
		return 1;
	}

	char ip[20];
	strcpy(ip, inet_ntoa(newnode->ip));

	while (cur->next != NULL){

		if(!strcmp((inet_ntoa(cur->ip)),ip)){
			return 0;
		}
		cur = cur->next;
	}
	cur->next = newnode;
	cur->next->next = NULL;

	return 1;

}


void kill_threads(pthread_t *tid, int i){

	//struct threads *cur = threads;
	
	/*while (cur != NULL){
		printf("Killing thread: %d\n", getpid());
		pthread_cancel(cur->tid);
		pthread_join(cur->tid, NULL);
		cur = cur->next;
	}*/

	while (i >= 0){
		//printf("Killing thread: %u\n", (unsigned int)tid[i]);
		pthread_cancel(tid[i]);
		pthread_join(tid[i], NULL);
		i--;
	}
	
	free(threads);

}

void *test(void *arg){
	
	int n = *((int *)arg);
	
	while(1){
		printf("Process ID: %u\t %d\n",(unsigned int)pthread_self(), pr[n].i);
		sleep(1);
	}

}

int check_existing_thread (char *ip){

	struct threads *cur = threads;
	char ip2[20];
	strcpy(ip2, ip);

	while (cur != NULL){
		if (!strcmp(ip2, inet_ntoa(cur->ip))){
			return 0;
		}
		cur = cur->next;
	}
	
	return 1;

}


void sig_alrm(int signo){

	Sendto(udp_send, msg1, sizeof(msg1), 0, (SA *)&p->mcastaddr, sizeof(p->mcastaddr));
	return;

}
