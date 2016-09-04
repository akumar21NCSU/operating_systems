/******************************************************************************
 *
 *  File Name........: master.c
 *
 *  Fortune
 *****************************************************************************/

/*........................ Include Files ....................................*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEF_PORT 9992;

main (int argc, char *argv[])
{
  char buf[512];
  char host[64];
  int s, p, fp, rc, len, port;
  int n;
  int hops;
	int i=0;
  struct hostent *hp, *ihp;
  struct sockaddr_in sin, incoming;
  int childarr[100];
	int child=0;

  /* read port number from command line */
  if ( argc < 4 ) {
    fprintf(stderr, "Usage: %s master <port-number> <number-of-players> <hops>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  n = atoi(argv[2]);
  hops = atoi(argv[3]);

  /* fill in hostent struct for self */
  gethostname(host, sizeof host);
  hp = gethostbyname(host);
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }

	printf("Potato Master on %s\n",hp->h_name);
	printf("Players = %d\n",n);
	printf("Hops = %d\n",hops);
	fflush(stdout);
  /* open a socket for listening
   * 4 steps:
   *	1. create socket
   *	2. bind it to an address/port
   *	3. listen
   *	4. accept a connection
   */

  /* use address family INET and STREAMing sockets (TCP) */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if ( s < 0 ) {
    perror("socket:");
    exit(s);
  }

  /* set up the address and port */
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
  
  /* bind socket s to address sin */
  rc = bind(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("bind:");
    exit(rc);
  }

  rc = listen(s, n);
  if ( rc < 0 ) {
    perror("listen:");
    exit(rc);
  }

  int playerSockets[n];
  int ports[n];
  unsigned long ips[n];

  /* accept connections */
  while (1) {
    len = sizeof(sin);
    p = accept(s, (struct sockaddr *)&incoming, &len);
    if ( p < 0 ) {
      perror("bind:");
      exit(rc);
    }

	playerSockets[child] = p;

	int port = incoming.sin_port;
	ports[child] = port;
	//printf("Port of child %d is %d\n",child,port);	

	unsigned long address = incoming.sin_addr.s_addr;
	//printf("Address is %ld\n",address);

	ips[child] = address;

	uint32_t pno = htonl(child);

	len = write(p, &pno, sizeof(uint32_t));
    if ( len != sizeof(uint32_t) ) {
      perror("write");
      exit(1);
    }
	
	uint32_t _n = htonl(n);

	len = write(p, &_n, sizeof(uint32_t));
    if ( len != sizeof(uint32_t) ) {
      perror("write");
      exit(1);
    }

    ihp = gethostbyaddr((char *)&incoming.sin_addr, 
			sizeof(struct in_addr), AF_INET);
	printf("player %d is on %s\n",child,ihp->h_name);
	fflush(stdout);

	child++;
	if(child == n)
		break;
  }




	sleep(3);

	int max = playerSockets[0];

	for(i=0;i<n;i++){

		int sock = playerSockets[i];
		if(playerSockets[i] > max)
			max = playerSockets[i];

		int r = (i+1);
		if(r > n-1)
			r = 0;
		
		unsigned long _rip = ips[r];
		uint32_t rip = htonl(_rip);

		int len = write(sock, &rip, sizeof(uint32_t));
		if ( len != sizeof(uint32_t) ) {
		  perror("write");
		  exit(1);
		}

		int _rport = ports[r];
		uint32_t rport = htonl(_rport);

		len = write(sock, &rport, sizeof(uint32_t));
		if ( len != sizeof(uint32_t) ) {
		  perror("write");
		  exit(1);
		}

		int _selfport = ports[i];
		uint32_t selfport = htonl(_selfport);

		len = write(sock, &selfport, sizeof(uint32_t));
		if ( len != sizeof(uint32_t) ) {
		  perror("write");
		  exit(1);
		}

	}

	if(hops == 0){
		close(s);
  
	  for(i=0;i<child;i++)
		close(playerSockets[i]);
	  exit(0);
	}
	

	max = max+1;

	srand(n);
	int first = rand()%n;

	printf("All players present, sending potato to player %d\n",first);
	fflush(stdout);

	uint32_t potato = htonl(hops);

	len = write(playerSockets[first], &potato, sizeof(uint32_t));
	if ( len != sizeof(uint32_t) ) {
	  perror("write");
	  exit(1);
	}
	
	
	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = 200;
    tv.tv_usec = 0;


	int retval;
	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	for(i=0;i<n;i++)
		FD_SET(playerSockets[i],&rfds);
		
	retval = select(max, &rfds, NULL, NULL, &tv);
	if (retval == -1)
    	perror("select()");
	else if (retval){
		printf("Trace of potato:\n");
		for(i=0;i<n;i++){
			if(FD_ISSET(playerSockets[i], &rfds)> 0){
				uint32_t _msgS;
				len = read(playerSockets[i], &_msgS, sizeof(uint32_t));
				if(len <=0){
					break;
				}
				int l = ntohl(_msgS);

				//printf("Trace size =%d\n",l);

				char trace[200];

				len = read(playerSockets[i], &trace, l);
				if(len <=0){
					break;
				}
				trace[l] = '\0';

				printf("%s\n", trace);
				break;
			}
		
		}		
	}else{
		printf("Timeout\n");
	}

  close(s);
  
  for(i=0;i<child;i++)
	close(playerSockets[i]);
  exit(0);
}

/*........................ end of listen.c ..................................*/
