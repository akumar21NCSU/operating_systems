/******************************************************************************
 *
 *  File Name........: speak.c
 *
 *  Description......:
 *	Create a process that talks to the listen.c program.  After 
 *  connecting, each line of input is sent to listen.
 *
 *  This program takes two arguments.  The first is the host name on which
 *  the listen process is running. (Note: listen must be started first.)
 *  The second is the port number on which listen is accepting connections.
 *
 *  Revision History.:
 *
 *  When	Who         What
 *  09/02/96    vin         Created
 *
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
#include <errno.h>

#define LEN	64

main (int argc, char *argv[])
{
  int s, rc, len, port;
  char host[LEN], str[LEN];
  struct hostent *hp;
  struct sockaddr_in sin;

  /* read host and port number from command line */
  if ( argc != 3 ) {
    fprintf(stderr, "Usage: %s <host-name> <port-number>\n", argv[0]);
    exit(1);
  }
  
  /* fill in hostent struct */
  hp = gethostbyname(argv[1]); 
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  port = atoi(argv[2]);

  /* create and connect to a socket */

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
  
  /* connect to socket at above addr and port */
  rc = connect(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("conneccct:");
    exit(rc);
  }

	uint32_t no;
	len = read(s, &no, sizeof(uint32_t));	
	int num = ntohl(no);
	printf("Connected as player %d\n", num);

	uint32_t _n;
	len = read(s, &_n, sizeof(uint32_t));	
	int n = ntohl(_n);

	char addrString[20];

	uint32_t _rip;
	len = read(s, &_rip, sizeof(uint32_t));
	long rip = ntohl(_rip);

	const char* ripaddr=inet_ntop(AF_INET,&rip,addrString,sizeof(addrString));
	if (ripaddr==0) {
		perror("failed to convert address to string.");
	}


	uint32_t _rport;
	len = read(s, &_rport, sizeof(uint32_t));
	int rport = ntohl(_rport);
	rport = rport+1;

	uint32_t _selfport;
	len = read(s, &_selfport, sizeof(uint32_t));
	int selfport = ntohl(_selfport);
	selfport = selfport+1;

	// listening

	  struct hostent *lhp;
	  struct sockaddr_in lsin;
	  char curHost[LEN];

	  gethostname(curHost, sizeof curHost);
	  //printf("Current host is %s",curHost);

	  lhp = gethostbyname(curHost);
	  if ( lhp == NULL ) {
		fprintf(stderr, "%s: host not found\n",curHost);
		exit(1);
	  }

	  int ls = socket(AF_INET, SOCK_STREAM, 0);
	  if ( ls < 0 ) {
		perror("socket:");
		exit(ls);
	  }

      lsin.sin_family = AF_INET;
	  lsin.sin_port = htons(selfport);
	  memcpy(&lsin.sin_addr, lhp->h_addr_list[0], lhp->h_length);
	  
	  /* bind socket s to address sin */
	  int lc = bind(ls, (struct sockaddr *)&lsin, sizeof(lsin));
	  if ( lc < 0 ) {
		perror("bind: Could not connect at this port. Please try again.");
		exit(lc);
	  }

	  lc = listen(ls, 3);
	  if ( lc < 0 ) {
		perror("listen:");
		exit(lc);
	  }

	// connecting to right player

	int rs = socket(AF_INET, SOCK_STREAM, 0);
	if ( rs < 0 ) {
		perror("socket:");
		exit(rs);
	}
	
	struct sockaddr_in sright;
	int ret;
	const char *tmpaddr = ripaddr;

	  struct hostent *rhp = gethostbyname(tmpaddr);

	  if ( rhp == NULL ) {
		fprintf(stderr, "%s: host not found\n", tmpaddr);
		exit(1);
	  }

	/* set up the address and port */
  sright.sin_family = AF_INET;
  sright.sin_port = htons(rport);
  memcpy(&sright.sin_addr, rhp->h_addr_list[0], rhp->h_length);

	sleep(2);
  
  /* connect to socket at above addr and port */
  ret = connect(rs, (struct sockaddr *)&sright, sizeof(sright));
  if ( ret < 0 ) {
    fprintf(stderr," err msg - %s\n",strerror(errno));
    exit(ret);
  }

	len = sizeof(lsin);
	struct sockaddr_in incoming;
    int lsock = accept(ls, (struct sockaddr *)&incoming, &len);
    if ( lsock < 0 ) {
      perror("bind:");
      exit(lsock);
    }	
  
	
 // select
	
	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = 100;
    tv.tv_usec = 0;

	int max=lsock;
	if(s > max)
		max = s;
	if(rs > max)
		max = rs;

	max = max+1;

	while(1){
		//printf("Repeating process\n");
		int retval;
		FD_ZERO(&rfds);
		FD_SET(lsock, &rfds);
		FD_SET(s, &rfds);
		FD_SET(rs, &rfds);

		retval = select(max, &rfds, NULL, NULL, &tv);
		if (retval == -1)
        	perror("select()");
		else if (retval){

			if(FD_ISSET(lsock, &rfds) > 0){

		// left
				//printf("Data from lsock\n");

				uint32_t _msgS;
				len = read(lsock, &_msgS, sizeof(uint32_t));
				if(len <=0)
					break;

				int msgS = ntohl(_msgS);

				//printf("Hop  Count = %d\n", msgS);

				uint32_t _oldl;
				len = read(lsock, &_oldl, sizeof(uint32_t));
				int oldl = ntohl(_oldl);

				//printf("oldl= %d\n",oldl);

				char trace[oldl+20];
				len = read(lsock, &trace, oldl);

				trace[oldl] ='\0';
				//printf("Trace so far= %s\n",trace);			

				strcat(trace,",");

				char playerString[10];	
				sprintf(playerString,"%d",num);
				//printf("Playerstring= %s\n",playerString);	

				strcat(trace,playerString);	
				//printf("Trace after %s\n",trace);

				int l = strlen(trace);


				if(msgS == 1){
					// end game.
					printf("I'm it\n");
					uint32_t lenTrace = htonl(l);					
					len = write(s, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(s,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}
					break;
				}
				msgS = msgS-1;

				srand(num*msgS);

				int neighbour = rand()%2;
				if(neighbour == 0){
					//printf("sending to left player\n");
					int next = num-1;
					if(next <0 )
						next = n-1;

					printf("Sending potato to %d\n",next);
					uint32_t potato = htonl(msgS);
					len = write(lsock, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}

					uint32_t lenTrace = htonl(l);					
					len = write(lsock, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(lsock,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}

					
				}else{
					//right
					//printf("Data from rsock\n");
					int next = num+1;
					if(next >=n )
						next = 0;

					printf("Sending potato to %d\n",next);
					uint32_t potato = htonl(msgS);
					len = write(rs, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}

					uint32_t lenTrace = htonl(l);					
					len = write(rs, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(rs,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}

				}

			}
				
			if(FD_ISSET(rs, &rfds)> 0){

	// right
				//printf("data from rs\n");

				uint32_t _msgS;
				len = read(rs, &_msgS, sizeof(uint32_t));
				int msgS = ntohl(_msgS);

				//printf("Hops = %d\n", msgS);

				uint32_t _oldl;
				len = read(rs, &_oldl, sizeof(uint32_t));
				int oldl = ntohl(_oldl);

				//printf("oldl = %d\n",oldl);

				char trace[oldl+6];
				len = read(rs, &trace, oldl);
			
				trace[oldl] ='\0';
				//printf("Trace so far= %s\n",trace);

				strcat(trace,",");

				char playerString[10];	
				sprintf(playerString,"%d",num);
	
				//printf("playerstring= %s\n",playerString);

				strcat(trace,playerString);	

				//printf("Trace after %s\n",trace);
				int l = strlen(trace);

				if(msgS == 1){
					// end game.
					printf("I'm it\n");
					uint32_t lenTrace = htonl(l);					
					len = write(s, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(s,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}
					break;
				}
				msgS = msgS-1;

				srand(num*msgS);

				int neighbour = rand()%2;
				if(neighbour == 0){
					// left
					int next = num-1;
					if(next <0 )
						next = n-1;

					printf("Sending potato to %d\n",next);
					uint32_t potato = htonl(msgS);
					len = write(lsock, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}

					uint32_t lenTrace = htonl(l);					
					len = write(lsock, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(lsock,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}
					
				}else{
					//right
					int next = num+1;
					if(next >=n )
						next = 0;

					printf("Sending potato to %d\n",next);
					uint32_t potato = htonl(msgS);
					len = write(rs, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}

					uint32_t lenTrace = htonl(l);					
					len = write(rs, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(rs,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}

				}

			}
		// master
			if(FD_ISSET(s, &rfds) > 0){
				FD_ZERO(&rfds);
				//printf("Data from s\n");

				uint32_t _msgS;
				len = read(s, &_msgS, sizeof(uint32_t));
				if(len <=0){
					//printf("No data\n");
					exit(0);
				}

				int msgS = ntohl(_msgS);
						
				char trace[200];
				trace[0]='\0';
				int len=0;

				char playerString[10];	
				sprintf(playerString,"%d",num);
	
				strcat(trace,playerString);	
				int l = strlen(trace);

				//printf("trace is %s and len is %d",trace,l);	

				//printf("Hops Count = %d\n", msgS);
			
				if(msgS == 1){
					// end game.

					printf("I'm it\n");
					uint32_t lenTrace = htonl(l);					
					len = write(s, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(s,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}
					break;
				}
				msgS = msgS -1;

				srand(num*msgS);

				int neighbour = rand()%2;
				if(neighbour == 0){
					// left
					int next = num-1;
					if(next <0 )
						next = n-1;

					printf("Sending potato to %d\n",next);
					uint32_t potato = htonl(msgS);
					len = write(lsock, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}


					uint32_t lenTrace = htonl(l);					
					len = write(lsock, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(lsock,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}

					
				}else{
					//right
					int next = num+1;
					if(next >=n )
						next = 0;
					
					printf("Sending potato to %d\n",next);
					fflush(stdout);					
					uint32_t potato = htonl(msgS);
					len = write(rs, &potato, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("write");
					  exit(1);
					}
			
					uint32_t lenTrace = htonl(l);					
					len = write(rs, &lenTrace, sizeof(uint32_t));
					if ( len != sizeof(uint32_t) ) {
					  perror("send");
					  exit(1);
					}

					len = write(rs,&trace,l);
					if ( len != l ) {
					  perror("send");
					  exit(1);
					}

				}

			}
		}
		    
		else{
		    printf("Timed out.\n");
			break;
		}
	}




// closing sockets

  close(s);
  close(rs);
  close(ls);
  exit(0);
}

/*........................ end of speak.c ...................................*/
