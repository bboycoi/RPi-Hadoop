
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <time.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include "zhelpers.h"
#include <pthread.h>


/* buffer for tun48/tap interface >= MTU 1500 */
#define BUFSIZE 1500 

#define CLIENT 0
#define SERVER 1
#define PORT 45678

/* epoch and time allocation */
int epoch = 8000, tAlloc[6] = {0, 2000, 3500, 5000, 6500, 8000};
int debug;
char *progname;
char config [256];

 /* allocates tun/tap device. most of simpletun.c*/
 
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}


 /* check-read checks for errors and exits if an error occurs */

int cread(int fd, char *buf, int n){
  
  int nread;

  if((nread=read(fd, buf, n)) < 0){
    perror("Reading data");
    exit(1);
  }
  return nread;
}


 /* check-write checks for errors and exits if an error occors*/

int cwrite(int fd, char *buf, int n){
  
  int nwrite;

  if((nwrite=write(fd, buf, n)) < 0){
    perror("Writing data");
    exit(1);
  }
  return nwrite;
}


 /* ensures read exactly n bytes, and puts them into "buf" unless EOF*/

int read_n(int fd, char *buf, int n) {

  int nread, left = n;

  while(left > 0) {
    if ((nread = cread(fd, buf, left)) == 0){
      return 0 ;      
    }else {
      left -= nread;
      buf += nread;
    }
  }
  return n;  
}


 /* do_debug: prints debugging message */

void do_debug(char *msg, ...){
  
  va_list argp;
  
  if(debug) {
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
  }
}


 /* my_err: prints custom error messages on stderr.*/

void my_err(char *msg, ...) {

  va_list argp;
  
  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}


/* Recv time allocation Config from controller */
void *recv_config(void *socket) {
  while (1){ 
    char *token;
    char config1 [100];
    int size = zmq_recv(socket, config1, 31, 0);
    if (size > 0){
      token = strtok(config1, ",");
      int k = 0;
      while(token != NULL && k<=5){
        tAlloc[k] = atoi(token);
        printf("%d ",tAlloc[k]);
        k++; 
        token = strtok(NULL, ",");
       }
       memset( config1, '\0', sizeof(config1) );
       epoch = tAlloc[k-1];
       s_sleep(2);
       printf("%d\n",epoch);	      	
    }
  }
}




/* Get last digit of IPaddr of WLAN0 */

int get_Last_IP_Digit() {
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /* get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* IP address attached to "wlan0" */
  strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

  ioctl(fd, SIOCGIFADDR, &ifr);

  close(fd);
  char ip[17];
  const char s[2] = ".";
  char *token;
  char lastdigit[4];
 
  sprintf(ip,"%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

  /* get the first token */
  token = strtok(ip, s);
    
  /* walk through other tokens */
  while( token != NULL ) {
   sprintf(lastdigit, "%s", token );
   token = strtok(NULL, s);
  }
 
  int n = atoi(lastdigit);
  return n;
}

 /* usage: prints usage and exits.*/

void usage(void) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
  fprintf(stderr, "%s -h\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "-i <ifacename>: Name of interface eg tun1 or tun2 ...\n");
  fprintf(stderr, "-s|-c <serverIP>: server mode (-s), or specify server address (-c <serverIP>)\n");
  fprintf(stderr, "-p <port>: port to listen on or connect to default 45678\n");
  fprintf(stderr, "-u|-a: use TUN (default) or TAP (-a)\n");
  fprintf(stderr, "-d: prints debug\n");
  fprintf(stderr, "-h: prints help \n");
  exit(1);
}

int main(int argc, char *argv[]) {
  
  int tap_fd, tap_fd1, option;
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  char *token;
  int maxfd;
  uint16_t nread, nwrite, plength;
  char buffer[BUFSIZE];
  struct timeval tv;
  struct sockaddr_in local, remote;
  char remote_ip[16] = "10.100.0.100";            /* IP string */
  unsigned short int port = 9999;
  int sock_fd, net_fd, optval = 1;
  socklen_t remotelen;
  int cliserv = -1;    /* Client or Server */
  unsigned long int tap2net = 0, net2tap = 0;
  pthread_t pt;
  
  progname = argv[0];
  
  /* Check user input options */
  while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
    switch(option) {
      case 'd':
        debug = 1;
        break;
      case 'h':
        usage();
        break;
      case 'i':
        strncpy(if_name,optarg, IFNAMSIZ-1);
        break;
      case 's':
        cliserv = SERVER;
        break;
      case 'c':
        cliserv = CLIENT;
        strncpy(remote_ip,optarg,15);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'u':
        flags = IFF_TUN;
        break;
      case 'a':
        flags = IFF_TAP;
        break;
      default:
        my_err("Unknown option %c\n", option);
        usage();
    }
  }

  argv += optind;
  argc -= optind;

  if(argc > 0) {
    my_err("Too many options!\n");
    usage();
  }

  if(*if_name == '\0') {
    my_err("Must specify interface name!\n");
    usage();
  } else if(cliserv < 0) {
    my_err("Must specify client or server mode!\n");
    usage();
  } else if((cliserv == CLIENT)&&(*remote_ip == '\0')) {
    my_err("Must specify server address!\n");
    usage();
  }

  /* initialize tun/tap interface */
  if ( (tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  do_debug("Successfully connected to interface %s\n", if_name);
  
  strncpy(if_name,"tun0", IFNAMSIZ-1);
  if ( (tap_fd1 = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    my_err("Error connecting to tun/tap interface tun0!\n");
    exit(1);
  }

  do_debug("Successfully connected to interface tun0 \n");


  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }
  if(cliserv == CLIENT) {  
  /* assign the destination address */
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip);
    remote.sin_port = htons(port);	
    int i=0;
    while(i <= 20){    
    /* connection request */
    	if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
      		//perror("connect()");
      		//exit(1);
      		sleep(10);
    	} else break;
    	if (++i == 20){
             perror("connect()");
	     printf("%s\n",remote_ip);	
	     exit(1);
	}
    }
    net_fd = sock_fd;
    do_debug("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));  
  }
/* 0MQ socket to talk to controller */
  
  //printf ("Collecting updates from controller...\n");
  void *context = zmq_ctx_new ();
  void *subscriber = zmq_socket (context, ZMQ_SUB);
  int rc = zmq_connect (subscriber, "tcp://10.100.0.100:9999");
  if(rc==0)printf("connected");
  assert (rc == 0);
  char *filter = "0,";
  rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,filter, strlen (filter));
  assert (rc == 0);
//  recv_config(subscriber);
   /* thread to check for configuration update */
  if (pthread_create(&pt, NULL, recv_config, subscriber) != 0){
    perror("Create Thread");
     exit(1); 
  }
  

  /* use select() to handle two descriptors at once */
  maxfd = (tap_fd > net_fd)?tap_fd:net_fd;

  int myTAlloc = get_Last_IP_Digit();
  while(1) {
//    recv_config(subscriber);
    int ret;
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tap_fd, &rd_set); 
//    FD_SET(net_fd, &rd_set);
    ret = select(maxfd + 1, &rd_set, NULL, NULL, 0);

	/* system call or library function call is blocked */
    if (ret < 0 && errno == EINTR){
      continue;
    }

    if (ret < 0) {
      perror("select()");
      exit(1);
    }
/*
    if(FD_ISSET(net_fd, &rd_set)) {
      nread = recv(net_fd, buffer, 25,0);
      if(nread != 0){
  	 char digit[6];
	 token = strtok(buffer, ",");
	 int k = 0;
	 while(token != NULL){
	  // sprintf(digit, "%s", token);
	   tAlloc[k++] = atoi(token);
	   printf("%d ",tAlloc[k-1]);
	   epoch = tAlloc[k-1];	
	   token = strtok(NULL, ",");
	 }
	 printf ("%d\n",epoch);
	
       }
    }
*/
    if(FD_ISSET(tap_fd, &rd_set)) {
      /* data from tun/tap: just read it and write it to the network */
            
      /* Obtain the time of day in millisecond */
      gettimeofday (&tv, NULL);
      int now = (tv.tv_usec) % epoch;
     // printf("%ld %d %d\n",now,now - tAlloc[myTAlloc-4],now - tAlloc[myTAlloc-1-4]);
      //printf("%d\n", now - tAlloc[myTAlloc-1-4]);
      /* time to send */
      if ((now - tAlloc[myTAlloc] < 0) && (now - (tAlloc[myTAlloc-1]+500) >= 0)){
        /* Read data from Tun interface */
       // printf ("sending...\n");
       // gettimeofday (&tv, NULL);
       // now = (tv.tv_usec/1000) % epoch;
       // printf("%ld %d %d\n",now,now - tAlloc[myTAlloc],now - tAlloc[myTAlloc-1]);
        nread = cread(tap_fd, buffer, BUFSIZE);
      
        tap2net++;
        do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);
       
        /* write length + packet */
        plength = htons(nread);
//        nwrite = cwrite(tap_fd, (char *)&plength, sizeof(plength));
        nwrite = cwrite(tap_fd1, buffer, nread);
       // gettimeofday (&tv, NULL);
//        now = (tv.tv_usec/1000) % epoch;
       // printf("%ld %d %d\n",now,now - tAlloc[myTAlloc],now - tAlloc[myTAlloc-1]);
        do_debug("TAP2NET %lu: Written %d bytes to the tap interface\n", tap2net, nwrite);
      } //else continue;
    }

  }
  zmq_close (subscriber);
  zmq_ctx_destroy (context);  
  return(0);
}
