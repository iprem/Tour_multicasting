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
