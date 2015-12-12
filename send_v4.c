#include	"ping.h"
#include	"a4.h"
#include	<net/if.h>
#include 	"unp.h"

void
send_v4(int sockfd, struct proto *pr)
{
	int			len, ip_flags[4], frame_length, n, i;
	struct icmp	*icmp;
	uint8_t *src_mac, *dst_mac, *ether_frame;
	char interface[40];
	struct ip iphdr;
	struct ifreq ifr;
	struct sockaddr_ll device;
	struct hwaddr Hwaddr;
	
	strcpy (interface, "eth0");	
	datalen = 56;

	src_mac = (uint8_t *)malloc (6 * sizeof(uint8_t)); 
	dst_mac = (uint8_t *)malloc (6 * sizeof(uint8_t));
	ether_frame = (uint8_t *)malloc (IP_MAXPACKET * sizeof(uint8_t));
	memset (&ifr, 0, sizeof (ifr));

	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	if (ioctl (sockfd, SIOCGIFHWADDR, &ifr) < 0) {
		perror ("ioctl() failed to get source MAC address ");
	}

	memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6);

	// Report source MAC address to stdout.
	/*printf ("MAC address for interface %s is ", interface);
	for (i=0; i<5; i++) {
		printf ("%02x:", src_mac[i]);
	}
	printf ("%02x\n", src_mac[5]);*/

	memset (&device, 0, sizeof (device));
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
		perror ("if_nametoindex() failed to obtain interface index ");
	}
	//printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);
	
	/* Here API has to be used to obtain MAC address */
	Hwaddr.sll_ifindex  = device.sll_ifindex;
	Hwaddr.sll_hatype = device.sll_hatype;
	Hwaddr.sll_halen = device.sll_halen;

	n = areq(&pr->sasend, sizeof(SA), &Hwaddr);

	if(n == -1){
		printf("Failed to obtain HW address\n");
		return;
	}
	
	//Obtain destination MAC Address vm1:00:0c:29:49:3f:5b
	/*dst_mac[0] = 0x00;
	dst_mac[1] = 0x0c;
	dst_mac[2] = 0x29;
	dst_mac[3] = 0x49;
	dst_mac[4] = 0x3f;
	dst_mac[5] = 0x5b;*/

	// Fill out sockaddr_ll.
	device.sll_family = AF_PACKET;
	memcpy (device.sll_addr, src_mac, 6);
	device.sll_halen = 6;

	memcpy(dst_mac, Hwaddr.sll_addr, 6);
	
	printf("HW address obtained: ");
	for (i=0; i<5; i++) {
    	printf ("%02x:", Hwaddr.sll_addr[i]);
  	}
  	printf ("%02x\n", Hwaddr.sll_addr[5]);
	

	iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);
	iphdr.ip_v = 4;
	iphdr.ip_tos = 0;
	iphdr.ip_len = htons (IP4_HDRLEN + ICMP_HDRLEN + datalen);
	iphdr.ip_id = htons (IDENTIFICATION);
	ip_flags[0] = 0;
	ip_flags[1] = 0;
	ip_flags[2] = 0;
	ip_flags[3] = 0;
	iphdr.ip_off = htons ((ip_flags[0] << 15)
						+ (ip_flags[1] << 14)
						+ (ip_flags[2] << 13)
						+  ip_flags[3]);
	iphdr.ip_ttl = 255;
	iphdr.ip_p = IPPROTO_ICMP;
	iphdr.ip_src = pr->sarecv;
	iphdr.ip_dst = pr->sasend;
	iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = pr->nsent++;
	memset(icmp->icmp_data, 0xa5, datalen);	/* fill with pattern */
	Gettimeofday((struct timeval *) icmp->icmp_data, NULL);

	len = 8 + datalen;		/* checksum ICMP header and data */
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

	/* Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + ICMP (header +  data))*/
	frame_length = 6 + 6 + 2 + IP4_HDRLEN + ICMP_HDRLEN+datalen;

	// Destination and Source MAC addresses
	memcpy (ether_frame, dst_mac, 6);
	memcpy (ether_frame + 6, src_mac, 6);

	ether_frame[12] = ETH_P_IP / 256;
	ether_frame[13] = ETH_P_IP % 256;

	memcpy (ether_frame + ETH_HDRLEN, &iphdr, IP4_HDRLEN);
	memcpy (ether_frame + ETH_HDRLEN + IP4_HDRLEN, sendbuf, ICMP_HDRLEN+datalen);

	Sendto(sockfd, ether_frame, frame_length, 0, (SA *)&device, sizeof(device));

	free (src_mac);
	free (dst_mac);
	free (ether_frame);

}
