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
#include "czmq.h"
#include "monitor.h"
#include <pthread.h>
#include "extern.c"

/* buffer for tun48/tap interface >= MTU 1500 */
#define BUFSIZE 1500  /*750000*/ 

#define CLIENT 0
#define SERVER 1
#define PORT 45678
pthread_cond_t qu_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t qu_full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t send_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qu_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qu_mutex1 = PTHREAD_MUTEX_INITIALIZER;

struct node
{
    int data_size;
    char *data;
    struct node *ptr;
};
struct queue
{
//     pthread_mutex_t qu_mutex;
     int id;
     int is_enqueuing;
     int count;
     struct node  *front,*rear,*temp,*front1;
	
};

int epoch = 8000, tAlloc[6] = {0, 2000, 3500, 5000, 6500, 8000};
int tap_fd1, debug; 
//int slot;//count[6];
int est = 0;
int priority_reducer = 0;
int c = 1; 
int hadoop_port_list [] = {8020,50010,50020,8030,8031,8032,8033,8040,8041.9000};
long now, end;
char *progname;
char config [256];
struct queue q[10];
int is_writting=0;
extern int data_in_q;

 /* do_debug: prints debugging message */
void do_debug(char *msg, ...){
  
  va_list argp;
  
  if(debug) {
        va_start(argp, msg);
        vfprintf(stderr, msg, argp);
        va_end(argp);
  }
}

/* Create an empty queue */
void create_queue(struct queue *q, int id)
{   
    q->id = id;
    q->front = q->rear = NULL;
    q->is_enqueuing = 0;
    q->count = 0;
   // q->qu_mutex = PTHREAD_MUTEX_INITIALIZER;
    //count[id]=0;
}
 
/* Returns queue size */
int queuesize(int id)
{	
	printf("\n Queue %d's size : %d",id, q[id].count);
	return q[id].count;
	
}
 
/* Enqueing the queue */
void enq(char *data, int id, int size )
{   
    q[id].is_enqueuing=1;
    if (q[id].rear == NULL)
    {	
	do_debug("\n Enqueue: empty queue\n");
	q[id].count = 0;
        q[id].rear = (struct node *)malloc(1*sizeof(struct node));
	q[id].rear->ptr = NULL;
	q[id].rear->data = malloc(size);
	memcpy(q[id].rear->data,data,size);
        q[id].rear->data_size = size;
	q[id].front = q[id].rear;
    }
    else
    {
	do_debug("\n Enqueued\n");
        q[id].temp=(struct node *)malloc(1*sizeof(struct node));    
	q[id].temp->ptr = NULL;
        q[id].temp->data = malloc(size);
        memcpy(q[id].temp->data,data,size);
        q[id].temp->data_size = size;
        q[id].rear->ptr = q[id].temp;
        q[id].rear = q[id].temp;
	
    }
    q[id].count++;
    do_debug("\ncount %d\n",q[id].count);
    q[id].is_enqueuing=0;
}
 
/* Dequeing the queue */
void deq(int id)
{
    q[id].front1 = q[id].front;
 
    if (q[id].front1 == NULL)
    {
        do_debug("\n Error: empty queue\n");
        return;
    }
    else
        if (q[id].front1->ptr != NULL)
        {
            /* Do Some Thing: Write to Tun0 (output operation)*/ 
            q[id].front1 = q[id].front1->ptr;
            do_debug("\n Dequed\n");
	    free(q[id].front->data);
            free(q[id].front);
            q[id].front = q[id].front1;
        }
        else
        {
            /* Do Some Thing: Write to Tun0 (output operation)*/ 
            do_debug("\n Dequed\n");
	    free(q[id].front->data);
            free(q[id].front);
            q[id].front = NULL;
            q[id].rear = NULL;
        }
        
	if(q[id].count > 0)
	  q[id].count--;
//	else  q[id].count=0;
	do_debug("\ncount %d\n",q[id].count);
}

int is_empty(){
	int total=0;
	int i = 1;
	for(i = 1; i<10; i++)
		total+= q[i].count;
//	if(total < 0)
//	  return 0;
	return total;
} 

/*Write to tun0 aka output operation*/
void *write_tun() {
	is_writting = 1;
	int nwrite;
	int total =is_empty(); 

  	int *p = get_link_quality();
  	int i =0;
  	float limit = 0.0;
  	int count = 0;
  	for(i = 1; i < 10;i++ ){
     		if (p[i] !=0){
          		limit += p[i];
          		count++;      
      		}
  	} 

  	limit = limit / count;
  	float real_limit = 0.0;
	do_debug("Start sending\n");
	while(1){
		int i = 1;
	        int k = 0;
//		real_limit = (limit * est)/1000000;
//		int sleep = (int)(slot/limit);
//		if (sleep > 10)
//			usleep(sleep - 10);		
//		usleep (250);
		data_in_q = is_empty();
		while(i<10){

			if(q[i].front != NULL){	
//				limit = (float) p[i];
//				real_limit = (limit * est)/1000000;						
				/*delay except ACK*/
			        is_writting = 1;
				int src,des;
				char src_port [2], des_port[2];
				memcpy( src_port, &q[i].front->data[20], 2 );
//				src_port[2] = '\0';
				src = src_port[0]<<8|src_port[1];
                                memcpy( des_port, &q[i].front->data[22], 2 );
//                                des_port[2] = '\0';
				des = des_port[0]<<8 |des_port[1]; 
//				printf("src:%d des:%d \n",src,des);
				int d = q[i].front->data[19];
				int hadoop_hb = 0;
				int j = 0;	
				for(j = 0; j < sizeof(hadoop_port_list)/sizeof(hadoop_port_list[0]);j++){
					if(des == hadoop_port_list[j]){
						hadoop_hb = hadoop_port_list[j];
						break;
					}
					else if(src == hadoop_port_list[j]){
							hadoop_hb = hadoop_port_list[j];
							break;
						}
				}
				// For nodes to AM and AM to Master 
				if (hadoop_hb <= 0 && ((des >= 55000 && des <= 55005) || (src >= 55000 && src <= 55005)))
					hadoop_hb = 1;
//				while(slot<=0 && q[i].front->data_size  > 500 && hadoop_hb <= 0){
//				while(slot<=0 && hadoop_hb <= 0 && d != 1 && q[i].front->data_size  > 150){
//				while(slot <= 0 && src == 13562){
				while(slot <= 0){
				  usleep(100);
				  c = 1;
				}
				/*Write to tun0 */
				pthread_mutex_lock(&qu_mutex);
                                if (is_empty() <= 0){
				  pthread_mutex_unlock(&qu_mutex);
				  usleep(100);
//				  if (is_empty() <= 0){
//					  slot =0;
//				  }else
//					pthread_mutex_lock(&qu_mutex);
                                 // pthread_cond_wait(&qu_empty_cond, &qu_mutex);
				}else{

//				if(q[i].front->data_size  > 500)
//				if(hadoop_hb <= 0 && q[i].front->data_size  > 150)
//					if (src == 13562)
//      						c++;
//      					if(c > (int)real_limit){
//						usleep(100);
         			//		c = 1;
         			//		slot = 0;      
//      					}else{
						nwrite = cwrite(tap_fd1, q[i].front->data, q[i].front->data_size);
						do_debug("TAP2NET: Written %d bytes to the net_fd %d interface\n", nwrite,i);
						c++;
						deq(q[i].id);
//				     	}
				}
				if(is_empty() < 450)
				  pthread_cond_signal(&qu_full_cond);
				pthread_mutex_unlock(&qu_mutex);
			}
/*			if( k <= 3 && priority_reducer!= 0 && i == priority_reducer && slot > 0 && q[i].front != NULL){
//			if( k <= 3 && slot > 0 && q[i].front != NULL){
//				int src1;
//                                char src_port1 [2];
//				memcpy( src_port1, &q[i].front->data[20], 2 );
//                                src1 = src_port1[0]<<8|src_port1[1];
//				if(src1 == 13562){
//					i = priority_reducer;
				k++;
//				}
//				else 
//					i++;
			}
			else
*/
			i++;
		}
		if(is_writting == 0)
		  usleep(100);
		is_writting = 0;
		total =is_empty();
	}
//	pthread_exit(0);
	return;
}

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

 /* my_err: prints custom error messages on stderr.*/
void my_err(char *msg, ...) {

  va_list argp;
  
  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}


/* Recv time allocation Config from controller */
void *recv_config(void *requester) {
  char config1 [100];
  struct timeval tv1;
  while (1){
//   s_sleep(1);
   memset( config1, '\0', sizeof(config1));
   if(zmq_recv(requester, config1, 31, 0) > 0){
    char *token;
//    do_debug("Got new slot:%s\n",config1);
    token = strtok(config1, ",");
    if(token != NULL){
     token = strtok(NULL, ",");
     slot = atoi(token);
     est = slot;
//     c = 1;
     token = strtok(NULL, ",");
     if(token != NULL)
        priority_reducer = atoi(token);
//     printf("%d\n",slot);
     while(slot > 0){
       usleep(100);
       pthread_mutex_lock(&qu_mutex); 
       slot = slot - 100; 
       pthread_mutex_unlock(&qu_mutex); 
     }
    }
    end = 0;
    slot=0;
//    c = 1;
    priority_reducer =  0;
    /* Inform Scheduler when transmition ended*/
    int size = zmq_send (requester,"1", strlen("1"), 0);
    if (size <= 0)
      perror("client sends error: ");
//    else do_debug("Sent ACK\n");
   }	 
  }
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
  
  int tap_fd, option;
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  int maxfd;
  struct timeval tv;
  struct timespec ts;
  uint16_t nread, nwrite, plength;
  //char buffer[BUFSIZE];
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
/* 0MQ socket to talk to controller */
  void *context = zmq_ctx_new ();
  void *requester = zmq_socket (context, ZMQ_REQ);
  int myIPLoc = get_Last_IP_Digit();
  char str [50];
  sprintf(str, "tcp://10.0.0.101:%d",myIPLoc*1111);
  int rc = zmq_connect (requester, str);
  if(rc==0)printf("connected");
  else perror("Can't connect to master");

  char config2 [10]; 
  // hand shake
  zmq_send (requester,"1", strlen("1"), 0);
  zmq_recv (requester, config2, 10, 0);
  zmq_send (requester,"1", strlen("1"), 0);
  if (pthread_create(&pt, NULL, recv_config, requester) != 0){
    perror("Create Thread");
    exit(1); 
  }

  /*Periodical report*/
  if (pthread_create(&pt, NULL, report_task, NULL) != 0){
    perror("Create report task Thread");
    exit(1); 
  }


  /* use select() to handle two descriptors at once */
  maxfd = (tap_fd > net_fd)?tap_fd:net_fd;

  /* Select timeout, wait up to 500 us. */
  gettimeofday (&tv, NULL);
  tv.tv_sec = 0;
  tv.tv_usec = 1000;
  
  int i = 1;
  for(i = 1; i<=9; i++)
	create_queue(&q[i],i);  
  
  if (pthread_create(&pt, NULL, write_tun, NULL) != 0){
                perror("Create Thread");
                exit(1); 
          }

  while(1) {
    int ret;
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tap_fd, &rd_set);
    ret = select(maxfd + 1, &rd_set, NULL, NULL,0);
    /* system call or library function call is blocked */
    if (ret < 0 && errno == EINTR){
      continue;
    }

//    if (ret < 0) {
//      perror("select()");
//	continue;
//      exit(1);
//    }
    if(FD_ISSET(tap_fd, &rd_set)){
      char buffer[BUFSIZE];
      nread = cread(tap_fd, buffer, BUFSIZE);
      int d = buffer[19];
      if(d <= 9 && d > 0){	
        do_debug("prepare to Enq queue%d\n",d);
	pthread_mutex_lock(&qu_mutex);
	while(is_empty() > 499){
	  usleep(100);
	  pthread_cond_wait(&qu_full_cond, &qu_mutex);
	}
	
	int src;
        char src_port [2];
        memcpy( src_port, &buffer[20], 2 );
        src = src_port[0]<<8|src_port[1];
	if (strstr(buffer, "GET /mapOutput"))
		printf("GET\n");
        if(src == 13562 && nread > 700){
        	enq(buffer,d, nread);
	}
	else
		nwrite = cwrite(tap_fd1, buffer, nread);	  
	//Wake up the next waiting output thread()
	pthread_cond_signal(&qu_empty_cond);
	//Unlock!!!
	pthread_mutex_unlock(&qu_mutex);
        tap2net++;
        do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);
      }
    }
  }
  zmq_close (requester);
  zmq_ctx_destroy (context);  
  return(0);
}


