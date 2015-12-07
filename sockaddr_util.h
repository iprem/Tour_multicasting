 #include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/un.h>

void init_sockaddr_un(struct sockaddr_un *addr, char * path );
void print_sockaddr_ll(struct sockaddr_ll *addr, int recv);
