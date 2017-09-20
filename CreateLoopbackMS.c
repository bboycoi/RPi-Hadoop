
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
#include "monitor.h"
#include "extern.c"


/* buffer for tun48/tap interface >= MTU 1500 */
#define BUFSIZE 1500 

#define CLIENT 0
#define SERVER 1
#define PORT 45678

/* epoch and time allocation */
int epoch = 6000, tAlloc[10] = {0,0,50000,50000,50000,50000,50000,50000,50000,50000};
int p [10] = {0,0,0,0,0,0,0,0,0,0};
int sp [10] = {0,0,0,0,0,0,0,0,0,0};
int reducer [10]= {0,0,0,0,0,0,0,0,0,0};
int slot;
int debug;
char *progname;
char config [256];
//extern int data_in_q;
//void *context = zmq_ctx_new ();
//void *rep_node2 = zmq_socket (context, ZMQ_REP);
//void *rep_node3 = zmq_socket (context, ZMQ_REP);
//void *rep_node4 = zmq_socket (context, ZMQ_REP);
//void *rep_node5 = zmq_socket (context, ZMQ_REP);
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
void *recv_config() {
  void *context = zmq_ctx_new ();
  void *socket = zmq_socket (context, ZMQ_SUB);
  int rc = zmq_connect (socket, "tcp://10.100.0.100:9990");
  if(rc==0)printf("connected");
  assert (rc == 0);
  char *filter = "0,";
  rc = zmq_setsockopt (socket, ZMQ_SUBSCRIBE,filter, strlen (filter));
  assert (rc == 0);
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


// server get periodical reports
void * process_report (void *args){
        //  Create a ROUTER socket. Default action is connect.  
//        int p[6] = {0,0,0,0,0};
//        void *context = zmq_ctx_new();
//        void *server = zmq_socket(context, ZMQ_ROUTER);
//        zmq_bind(server, "tcp://10.0.0.101:5671"); /**/
	zsock_t * server = zsock_new_router("tcp://10.0.0.101:5671");

        while(1){
                char *content = zstr_recv (server);
//                printf("%s\n",content);
                const char s[2] = ".";
                char *token;
                int nodeId = 0;
                int size = 0;
                int speed = 0;
                int count = 0;
                token = strtok(content, s); //Node ID
                if(token != NULL){
                        nodeId = atoi(token);
                        token = strtok(NULL,s); //data size
                        size = atoi(token); 
                        p[nodeId] = size;
                        token = strtok(NULL,s); // is reducer
                        reducer[nodeId] = atoi(token);
                        token = strtok(NULL,s);   // speed rate packet per second
                        while(token != NULL){   
                                int tk = atoi(token);
                                speed += tk;
                                if(tk!=0)
                                        count++;
                                token = strtok(NULL,s);
                        }
                        if(count != 0){
                                speed = speed/count;
				sp[nodeId] = speed;
			}
/*
                        int i = 0;
                                                for(i = 1; i < sizeof(p)/sizeof(int);i++ ){
                                if (i == nodeId){
                                        p[i] = size;
                                }
                        } 
*/
	                printf("node%d,reduce=%d, size:%d, speed:%d\n", nodeId,reducer[nodeId],p[nodeId],speed);
        	        printf("-----------------------------------------------------------\n");
                }
                free(content);
        }
        zsock_destroy(&server);
//        zmq_close(server);
//        zmq_ctx_destroy(context);
}

int max_array(int array[], int array_size){
   int largest = array[0];
   int i = 0;
   int max = 0;
   for (i = 1; i < array_size; i++)
   {
     if (largest < array[i]){
         largest = array[i];
	 max = i;
     } 
   }
   return max;
}


/*Scheduller communication*/
void *send_config() {
  char config1[50];
  char ack[2];
  struct timeval tv;
  long now, end;
  int silence = 0;
  pthread_t pt;
  // Set broadcast interface to ETH0
  zsys_set_interface ("eth0");
  zactor_t *speaker = zactor_new (zbeacon, NULL);
  zsock_send (speaker, "si", "CONFIGURE", 2018);
  char *hostname = zstr_recv (speaker);
  if (!*hostname) {
        printf ("not support UDP broadcasting)\n");
        zactor_destroy (&speaker);
        free (hostname);
        return;
  }
  free (hostname);
  void *context = zmq_ctx_new ();
  void *rep_node2 = zmq_socket (context, ZMQ_REP);
  void *rep_node3 = zmq_socket (context, ZMQ_REP);
  void *rep_node4 = zmq_socket (context, ZMQ_REP);
  void *rep_node5 = zmq_socket (context, ZMQ_REP);
  void *rep_node6 = zmq_socket (context, ZMQ_REP);
  void *rep_node7 = zmq_socket (context, ZMQ_REP);
  void *rep_node8 = zmq_socket (context, ZMQ_REP);
  void *rep_node9 = zmq_socket (context, ZMQ_REP);


  int rc = zmq_bind(rep_node2, "tcp://10.0.0.101:2222");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node3, "tcp://10.0.0.101:3333");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node4, "tcp://10.0.0.101:4444");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node5, "tcp://10.0.0.101:5555");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node6, "tcp://10.0.0.101:6666");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node7, "tcp://10.0.0.101:7777");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node8, "tcp://10.0.0.101:8888");/**/
  assert (rc == 0);
  rc = zmq_bind(rep_node9, "tcp://10.0.0.101:9999");/**/
  assert (rc == 0);

  zmq_recv (rep_node2, config1, 10, 0);
  printf("node2 connected\n");
  zmq_recv (rep_node3, config1, 10, 0);
  printf("node3 connected\n"); 
  zmq_recv (rep_node4, config1, 10, 0);
  printf("node4 connected\n"); 
  zmq_recv (rep_node5, config1, 10, 0);
  printf("node5 connected\n"); 
  zmq_recv (rep_node6, config1, 10, 0);
  printf("node6 connected\n"); 
  zmq_recv (rep_node7, config1, 10, 0);
  printf("node7 connected\n"); 
  zmq_recv (rep_node8, config1, 10, 0);
  printf("node8 connected\n");  
  zmq_recv (rep_node9, config1, 10, 0);
  printf("node9 connected\n"); 
/* Process report*/
  if (pthread_create(&pt, NULL, process_report, NULL) != 0){
    perror("Create response_task Thread ");
    exit(1); 
  }
  zmq_send (rep_node2,"1", strlen("1"), 0);
  zmq_send (rep_node3,"1", strlen("1"), 0);
  zmq_send (rep_node4,"1", strlen("1"), 0);
  zmq_send (rep_node5,"1", strlen("1"), 0);
  zmq_send (rep_node6,"1", strlen("1"), 0);
  zmq_send (rep_node7,"1", strlen("1"), 0);
  zmq_send (rep_node8,"1", strlen("1"), 0);
  zmq_send (rep_node9,"1", strlen("1"), 0);

  zmq_recv (rep_node2, config1, 10, 0);
  zmq_recv (rep_node3, config1, 10, 0);
  zmq_recv (rep_node4, config1, 10, 0);
  zmq_recv (rep_node5, config1, 10, 0);
  zmq_recv (rep_node6, config1, 10, 0);
  zmq_recv (rep_node7, config1, 10, 0);
  zmq_recv (rep_node8, config1, 10, 0);
  zmq_recv (rep_node9, config1, 10, 0);

  int  p_size = sizeof(p)/sizeof(int);
 // int  cur[p_size-1] = {0,0,0,0,0};
  int pri = 0;
  while (1){ 
   zsock_send (speaker, "ssi", "PUBLISH","1", sizeof(char)*2, 10);
   usleep(1000);
   zstr_sendx (speaker, "SILENCE", NULL);
   int i = 0;
   int check = 0;
   usleep(50000);
   int pri_job = 0;
   int max_progress = 0;
   int job_temp = 0;
   int pri_reducer = 0;
   int current_send_data_size = 0;
   char *trash;
   FILE *fp = fopen("jobProgress","r");
   if (fp!=NULL){
	char line[128];
	while (fgets ( line, sizeof line, fp ) != NULL ){
		const char s[2] = ":";
                char *token;
                token = strtok(line, s);
                if(token != NULL){
			job_temp = (int)strtol(token, (char**)NULL,10);
			printf("job:%s ",token);
		}
		token = strtok(NULL,s);
		if(token != NULL){
			printf(":%s  \n",token);
			if ((int)strtol(token, (char**)NULL,10) > max_progress){
				max_progress = (int)strtol(token, (char**)NULL,10);
				pri_job = job_temp;
			}
		}
		memset( line, '\0', sizeof(line));
	}
	fclose(fp);
	printf("priority job:%d %d% \n",pri_job,max_progress);
   }

   for(i = 1; i < sizeof(p)/sizeof(int);i++ ){
     if(p[i]!=0){
        check++;
	break;
     }
   }

   if (check==0){
      for(i = 1; i < sizeof(tAlloc)/sizeof(int);i++ ){
	tAlloc[i] = 50000;
      }	
      usleep(500000);
   }else{
//	if (pri == 0 || p[pri] == 0){ //|| current_send_data_size > p[pri]){
//		pri = 0;
//       	pri = max_array(p,p_size);
		pri++;
                if(pri > p_size - 1)
                        pri = 2;

		while(p[pri] <= 0 && pri < p_size){
			pri++;
			if(pri >= p_size){
		 		pri = 2;
			}
		}	

//	}

        for(i = 1; i < sizeof(tAlloc)/sizeof(int);i++ ){
	   if(reducer[i] != 0){
//		tAlloc[i] = 1000;
		if(pri_job != 0 && max_progress < 100 && reducer[i] == pri_job)
		    pri_reducer = i;
	   }
	   if(i == pri){
		    tAlloc[pri] = 2000000;
//	    	    current_send_data_size  = p[pri];
           }
		else 
		       tAlloc[i] = 0;
        }

//        float time = 0.0;
//        time = (1.0 * p[pri])/(1.0 * sp[pri] * 1500);
//        tAlloc[pri] = (int)(time * 1000000); 
   }
   printf("%d,%d,%d,%d,%d,%d,%d,%d\n",tAlloc[2],tAlloc[3],tAlloc[4],tAlloc[5],tAlloc[6],tAlloc[7],tAlloc[8],tAlloc[9]);

   int k = 1;
   slot = tAlloc[k];
//   usleep(slot);
   slot = 0;
   k++;
   while(k<=9){
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size2 = zmq_send (rep_node2, config1, strlen (config1), 0);
        if (size2 <= 0)
           perror("Master2 sends: ");
        if(zmq_recv(rep_node2, ack, 2, 0) <= 0)
           do_debug("Ack2\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size3 = zmq_send (rep_node3, config1, strlen (config1), 0);
        if (size3 <= 0)
           perror("Master3 sends: ");
        if(zmq_recv(rep_node3, ack, 2, 0) <= 0)
           do_debug("Ack3\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size4 = zmq_send (rep_node4, config1, strlen (config1), 0);
        if (size4 <= 0)
           perror("Master4 sends: ");
        if(zmq_recv(rep_node4, ack, 2, 0) <= 0)
           do_debug("Ack4\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size5 = zmq_send (rep_node5, config1, strlen (config1), 0);
        if (size5 <= 0)
           perror("Master5 sends: ");
        if(zmq_recv(rep_node5, ack, 2, 0) <= 0)
          do_debug("Ack5\n");
     }
     k++;    
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size6 = zmq_send (rep_node6, config1, strlen (config1), 0);
        if (size6 <= 0)
           perror("Master6 sends: ");
        if(zmq_recv(rep_node6, ack, 2, 0) <= 0)
          do_debug("Ack6\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size7 = zmq_send (rep_node7, config1, strlen (config1), 0);
        if (size7 <= 0)
           perror("Master7 sends: ");
        if(zmq_recv(rep_node7, ack, 2, 0) <= 0)
          do_debug("Ack7\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size8 = zmq_send (rep_node8, config1, strlen (config1), 0);
        if (size8 <= 0)
           perror("Master8 sends: ");
        if(zmq_recv(rep_node8, ack, 2, 0) <= 0)
          do_debug("Ack8\n");
     }
     k++;
     memset( config1, '\0', sizeof(config1));
     if(tAlloc[k] != 0){
        sprintf(config1,"node%d,%d,%d\0",k,tAlloc[k],pri_reducer);
        int size9 = zmq_send (rep_node9, config1, strlen (config1), 0);
        if (size9 <= 0)
           perror("Master9 sends: ");
        if(zmq_recv(rep_node9, ack, 2, 0) <= 0)
          do_debug("Ack9\n");
     }
     k++;
   }
  }
  zmq_close (rep_node2);
  zmq_close (rep_node3);
  zmq_close (rep_node4);
  zmq_close (rep_node5);
  zmq_close (rep_node6);
  zmq_close (rep_node7);
  zmq_close (rep_node8);
  zmq_close (rep_node9);

  zactor_destroy (&speaker);
  zmq_ctx_destroy (context);  
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
  unsigned short int port = 1234;
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
/* 0MQ socket to talk to controller  
  if (pthread_create(&pt, NULL, recv_config, NULL) != 0){
    perror("Create Thread");
    exit(1); 
  }
*/
  if (pthread_create(&pt, NULL, send_config, NULL) != 0){
    perror("Create Thread");
    exit(1); 
  }

  /* use select() to handle two descriptors at once */
  maxfd = (tap_fd > net_fd)?tap_fd:net_fd;

  int myTAlloc = get_Last_IP_Digit();
  while(1) {
    int ret;
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tap_fd, &rd_set); 
    ret = select(maxfd + 1, &rd_set, NULL, NULL, 0);

	/* system call or library function call is blocked */
    if (ret < 0 && errno == EINTR){
      continue;
    }

//    if (ret < 0) {
//      perror("select()");
//      exit(1);
//    }
    if(FD_ISSET(tap_fd, &rd_set)){ //&& slot !=0) {
        /* Read data from Tun interface */
        nread = cread(tap_fd, buffer, BUFSIZE);      
//	while(slot==0 && nread > 40){usleep(1);}
//        do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);
        /* write length + packet */
        nwrite = cwrite(tap_fd1, buffer, nread);
//        do_debug("TAP2NET %lu: Written %d bytes to the tap interface\n", tap2net, nwrite);
     // }
    }
  }
//  zmq_close (subscriber);
//  zmq_ctx_destroy (context);  
  return(0);
}
