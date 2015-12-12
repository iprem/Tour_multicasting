=============================
#Authors
	: Michael Liuzzi
	: Prem Shankar
#Assignment 4
#Date:	 11th December, 2015
=============================


==================
USER DOCUMENTATION
==================
The submission includes 
	1. Header file: a4.h hw_addrs.h ping.h sockaddr_util.h

	2. C files: tour.c arp.c send_v4.c readloop.c areq.c a4_utils.c get_hw_addrs.c proc_v4.c send_v4.c sockaddr_util.c tv_sub.c

Execute the following from source directory

    1. make						# Compile and generate all the executable files
    
    2. ./deploy_app <arp> <tour># Deploy the ARP and tour process on vm nodes
	
	3. ./start_app arp			# Run arp process on all the nodes
	
    3. ./tour <node1 node2 ...>	# Start tour process on all the nodes to be visited and provide list of nodes for node which starts tour


=====================
TESTING DOCUMENTATION
=====================

1. Run the ARP process by executing "./arp" on each node.
2. Run tour process by executing "./tour" on each node other than the node starting the tour
3. Run tour process on node which starts the tour by executing "./tour <node1 node2 ...>"
	- The source node starts the tour by sending message to nodes node1, node2, ... in order which is being received and displayed by the subsequent nodes.
	- The nodes after forwarding the tour message joins multicast group and starts pinging its previous node in tour.
	- The nodes starts recieving ping messages which are displayed on screen.
	- Once last node in the tour is reached, tour ends, pinging activities are halted and it sends message on multicast address.
	- As all the nodes have joined multicast address, every node recieves the message and send back on the same address saying they are part of the group.


====================
SYSTEM DOCUMENTATION
====================

1. An application is developed that uses raw IP sockets to ‘walk’ around an ordered list of nodes (given as a command line argument at the ‘source’ node, 
	which is the node at which the tour was initiated), in a manner similar to the IP SSRR (Strict Source and Record Route) option.
2. At each node, the application pings the preceding node in the tour. Ping ICMP echo request messages are sent through a SOCK_RAW-type PF_PACKET socket and
	implements ARP functionality to find the Ethernet address of the target node.
3. Finally, when the ‘walk’ is completed, the group of nodes visited on the tour exchanges multicast messages.
4.  The application consist of two process modules, a ‘Tour’ application module (which implements all the functionality outlined above, except for ARP activity) and an ARP module.

                                     ********************* TOUR ****************************
1. If the node is one that starts the tour, the program parses the command line arguments and adds it to payload to be sent to other nodes in the tour.
2. The program makes the node join multicast group and then send the payload forward along the tour.
3. If the node is not a node which starts the tour, it just waits to receive reply from previous node in the tour.
4. The program uses select to wait on rt, pg and udp_recv sockets which prints as the messages arrive.
5. When the last node is reached, pinging halts and then multicast messages are exchanged.


									********************** ARP *****************************
1. It uses the get_hw_addrs function of Assignment 3 to explore its node’s interfaces and build a set of <IP address ,HW address> matching pairs for all eth0 interface IP addresses
2. The module creates two sockets: a PF_PACKET socket and a Unix domain socket.
3. PF-PACKET socket is used for sending and receiving ARP requests and replies for finding hardware address  of destination.
4. The UNIX domain socket is  a listening socket bound to a ‘well-known’ sun_path file. This socket will be used to communicate with the function areq that is implemented in the Tour module
5. The ARP module then sits in an infinite loop, monitoring these two sockets.
6. As ARP request messages arrive on the PF_PACKET socket, the module processes them, and responds with ARP reply messages as appropriate.


									*********************** AREQ API ************************
1. The API is for communication between the Tour process and the ARP process. It consists of a single function, areq, implemented in the Tour module. 
2. areq is called by send_v4 function of the application every time the latter want to send out an ICMP echo request message.
3. areq creates a Unix domain socket of type SOCK_STREAM and connects to the ‘well-known’ sun_path file of the ARP listening socket. 
4. It sends the IP address from parameter IPaddr and the information in the three fields of parameter HWaddr to ARP. 
5. When the ARP module receives a request for a HW address from areq through its Unix domain listening socket, it first checks if the required HW address is already in the cache.
6.  If so, it can respond immediately to the areq and close the Unix domain connection socket. Else : it makes an ‘incomplete’ entry in the cache.


