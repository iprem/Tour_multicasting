#include "a4.h"

void findHostName(char *ip, char *host)
{
	
  struct hostent *he;
  struct in_addr ipv4addr;
  
  inet_pton(AF_INET, ip, &ipv4addr);
  if( (he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET)) == NULL )
    printf("Error: gethostbyaddr\n");
	
  strcpy(host,he->h_name);
  
}

void findOwnIP( char *own_ip ){
	
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	int eth_zero = 0;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		
		if(strcmp("eth0", hwa->if_name) == 0)
		  {
			if ( (sa = hwa->ip_addr) != NULL)
			  {
			    strcpy(own_ip,Sock_ntop_host(sa, sizeof(*sa)));
			    eth_zero = 1;
			    break;
			  }
		  }
	}
	if (!eth_zero)
	  printf("Error: No IP address found for eth0\n");

	free_hwa_info(hwahead);

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



