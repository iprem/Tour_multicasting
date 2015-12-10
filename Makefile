CC = gcc
CFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g  -D_REENTRANT -Wall -L /users/cse533/Stevens/unpv13e/ -o 
LIBS = -lunp -lpthread -lrt
CLEANFILES = tour *~ *.o ud_* 


all: tour

tour : tour.c get_hw_addrs.c a4_utils.c sig_alrm.c readloop.c proc_v4.c send_v4.c 
	$(CC) $(CFLAGS) tour tour.c get_hw_addrs.c a4_utils.c sig_alrm.c readloop.c proc_v4.c send_v4.c $(LIBS)

#client : client.c get_hw_addrs.c sockaddr_util.c send.c a3_utils.c
#	$(CC) $(CFLAGS) client client.c get_hw_addrs.c sockaddr_util.c send.c recv.c a3_utils.c $(LIBS)


clean:
	rm -f $(CLEANFILES)
