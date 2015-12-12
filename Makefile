CC = gcc
CFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g  -D_REENTRANT -Wall -L /users/cse533/Stevens/unpv13e/ -o
LIBS = -lunp -lpthread -lrt
CLEANFILES = tour *~ *.o ud_*


all: tour arp

tour : tour.c get_hw_addrs.c a4_utils.c proc_v4.c readloop.c send_v4.c areq.c sockaddr_util.c
	$(CC) $(CFLAGS) tour tour.c get_hw_addrs.c a4_utils.c proc_v4.c readloop.c send_v4.c areq.c sockaddr_util.c $(LIBS)

arp: arp.c
	$(CC) $(CFLAGS) arp arp.c sockaddr_util.c  get_hw_addrs.c a4_utils.c $(LIBS)


clean:
	rm -f $(CLEANFILES)
