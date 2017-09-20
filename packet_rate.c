#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> // for exit()
#include<string.h> //for memset
#include<stdint.h>
#include<time.h>
#include<signal.h>
#include<sys/ipc.h>
#include<unistd.h>
#include<net/if.h>

#include "czmq.h"
 
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_ntoa()
#include<net/ethernet.h>
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header
 
void process_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
void *recv_broadcast(void *listener);
int get_last_digit(char ip[17]);
void sigint(int a);
int get_Last_IP_Digit();
int get_digit_from_content(char content[100],int last_digit);
void print_time();

FILE *logfile;
struct sockaddr_in source,dest;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j; 
int64_t expiry;
int b_data[9] = {0,0,0,0,0,0,0,0,0}; 
int local_data[9] = {0,0,0,0,0,0,0,0,0}; 
int from_others[9] = {0,0,0,0,0,0,0,0,0};
int eth_Ipaddr [9] = {101,102,103,104,105,106,107,108,109};
char tosend[9];

zactor_t *speaker;
zactor_t *listener;

int main()
{
    if (signal(SIGINT, sigint) == SIG_ERR) {
         printf("SIGINT install error\n");
         exit(1);
    }
    if (signal(SIGTSTP, sigint) == SIG_ERR) {
         printf("SIGINT install error\n");
         exit(1);
    }

    pthread_t pt;
    // Set broadcast interface to ETH0
    zsys_set_interface ("eth0");
    printf("\n");
    // Initiate zbeacon
    speaker = zactor_new (zbeacon, NULL);
    zsock_send (speaker, "si", "CONFIGURE", 2017);
    char *hostname = zstr_recv (speaker);
    if (!*hostname) {
        printf ("not support UDP broadcasting)\n");
        zactor_destroy (&speaker);
        free (hostname);
        return;
    }
    free (hostname);
    //initiate listener
    listener = zactor_new (zbeacon, NULL);
    assert (listener);
   // zstr_sendx (listener, "VERBOSE", NULL);
    zsock_send (listener, "si", "CONFIGURE", 2017);
    hostname = zstr_recv (listener);
    assert (*hostname);
    free (hostname);

    pcap_if_t *alldevsp , *device;
    pcap_t *handle; //Handle of the device that shall be sniffed
 
    char errbuf[100] , *devname , devs[100][100];
    int count = 1 , n;
     
    //First get the list of available devices
    printf("Finding available devices ... ");
    if( pcap_findalldevs( &alldevsp , errbuf) )
    {
        printf("Error finding devices : %s" , errbuf);
        exit(1);
    }
    printf("Done");
     
    devname = "wlan0";
     
    //Open the device for sniffing
    printf("Opening device %s for sniffing ... " , devname);
    handle = pcap_open_live(devname , 65536 , 1 , 0 , errbuf);
     
    if (handle == NULL) 
    {
        fprintf(stderr, "Couldn't open device %s : %s\n" , devname , errbuf);
        exit(1);
    }
    printf("Done\n");
     
    //construct data to send 
    int i = 0;
    for (i = 0 ; i < 9 ; ++i) {
        tosend[i] = htonl(b_data[i]) ;
    }

    //Start broacasting
    //  We will listen to anything (empty subscription)
    zsock_send (listener, "sb", "SUBSCRIBE", "", 0);
    if (pthread_create(&pt, NULL,recv_broadcast, listener) != 0){
        perror("Create Thread");
        exit(1); 
    }
    printf("capturing loop!\n");
    //Put the device in sniff loop
    expiry = zclock_mono () + 1000;
    pcap_loop(handle , -1 , process_packet , NULL);
    zstr_sendx (speaker, "SILENCE", NULL);
    zactor_destroy (&speaker);
    zactor_destroy (&listener);	
    return 0;   
}

// Interupt Handling
void sigint(int a)
{
    zstr_sendx (speaker, "SILENCE", NULL);    
    zactor_destroy (&speaker);
    zactor_destroy (&listener); 
    kill (getppid(), 9);
    exit(0);

} 

void print_time()
{
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s\n",buffer);
}

//Listien to broadcast message
void *recv_broadcast(void *listener){
    while(1){
    	char *ipaddress = zstr_recv (listener);
    	if (ipaddress){
	  char *content = zstr_recv (listener);
	  int remote_eth_ip = get_last_digit(ipaddress);
	  if(remote_eth_ip != get_Last_IP_Digit(1)) {
  	    const char s[2] = ",";
	    //char number[4];
//            printf("From: %d\n ",this_eth_ip);
//    	    printf("content: %s\n",content);
            char * token = strtok(content,s);
	    int this_ip = get_Last_IP_Digit(2);
//	    printf("%d\n ",this_ip);

	    int number1 = get_digit_from_content(content,this_ip);
            int i = 0;
	    for(i = 0;i < sizeof(eth_Ipaddr)/ sizeof(int); i++){
		if(eth_Ipaddr[i] == remote_eth_ip && get_Last_IP_Digit(1) != eth_Ipaddr[i]){
			if((number1 > from_others[i]) && number1 > 100)
                        from_others[i] = number1;
		}
	    }
/*
	    switch(this_eth_ip)
	    {
		case 102:
		   // from_others[1] = atoi(number1);
		    if(number1 > from_others[1])
		    	from_others[1] = number1;
		   // printf("%d ",number1); printf("%d\n",from_others[1]);
		    break;

		 case 103:
	           // from_others[2] = atoi(number1);
		    if(number1 > from_others[2])
                        from_others[2] = number1;
		   // printf("%d ",number1); printf("%d\n",from_others[2]);
                    break;

		 case 104:
		   // from_others[3] = atoi(number1);
                    if(number1 > from_others[3])
                        from_others[3] = number1;
		   // printf("%d ",number1); printf("%d\n",from_others[3]);
                    break;

		 case 105:
		   // from_others[4] = atoi(number1);
                    if(number1 > from_others[4])
                        from_others[4] = number1;
		    //printf("%d ",number1); printf("%d\n",from_others[4]);
                    break;

                 default: //Some Other IP Addresses
                    break;

	    }
*/	    int check = 0;
	    i = 0;
            for(i = 0;i < sizeof(from_others)/ sizeof(int); i++){
		check+= from_others [i];
	    }
//	    if(from_others[1] != 0 || from_others[2] != 0 || from_others[3] != 0 || from_others[4] != 0){
	    if(check !=0){
	        int j =0;
/*		print_time();
	    	printf("received: ");
	    	for(j = 0; j < 5; j++){
	             printf("%d ", from_others[j]);
	    	}
	    	printf("\n");
		j = 0;
		printf("Local: ");
                for(j = 0; j < 5; j++){
                     printf("%d ", local_data[j]);
                }
                printf("\n");
*/
		logfile=fopen("speed_log.txt","w");
    		if(logfile==NULL) 
    		{
        		printf("Unable to create file.");
    		}
		char *str_tofile = malloc(200*sizeof(char));
        	char c_num [5];
        	int k = 0; 
        	for (k = 0 ; k < 9 ;k++) {
             	    sprintf(c_num,"%d",from_others[k]);
            	    strcat(str_tofile,c_num);
            	    memset(c_num, 0, sizeof (c_num)); 
                    if(k < 8)
                        strcat(str_tofile,".");
            	    else 
                        strcat(str_tofile,"\0");
        	}
		fprintf(logfile,"%s",str_tofile);
		fclose(logfile);
		memset(str_tofile, 0, sizeof (str_tofile));     
	     }
	  }
	  zstr_free(&content);
	}
	zstr_free(&ipaddress);
    }
    return;
}

int get_digit_from_content(char content[100],int last_digit){
    const char s[2] = ".";
    char *token;
    char digit[4];
 
    /* get the first token */
    token = strtok(content, s);
    int i = 0;
    /* walk through other tokens */
    while(i<last_digit && token != NULL ) {
       sprintf(digit, "%s", token );
//       printf("%s %s,",token,digit);
       token = strtok(NULL, s);
       i++;
    }
 
    int n = atoi(digit);
    return n;
}

/* Get last digit of IPaddr of WLAN0 */
int get_Last_IP_Digit( int code) {
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /* get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* IP address attached to "wlan0" */
  if(code == 1)
      strncpy(ifr.ifr_name,"eth0", IFNAMSIZ-1);
  else if(code == 2)
      strncpy(ifr.ifr_name,"wlan0", IFNAMSIZ-1);

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

int get_last_digit(char ip[17]){
    const char s[2] = ".";
    char *token;
    char lastdigit[4];
 
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

//Sniffing Loop 
void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
    zstr_sendx (speaker, "SILENCE", NULL);    
    int size = header->len;
    unsigned short iphdrlen;
     
    //Get the IP Header part of this packet , excluding the ethernet header
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ihl*4;
    struct tcphdr *tcph=(struct tcphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
    int src_port, des_port;

    if(iph->protocol == 6) //Check the TCP Protocol...
    {           
        char src[20],des[20];
	memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = iph->saddr;
	sprintf(src,"%s",inet_ntoa(source.sin_addr));

        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = iph->daddr;
	sprintf(des,"%s",inet_ntoa(dest.sin_addr));

        src_port = ntohs(tcph->source);
        des_port = ntohs(tcph->dest);
	int this_packet_src_ip = get_last_digit(src);
        int this_packet_des_ip = get_last_digit(des);


	if(this_packet_src_ip  != get_Last_IP_Digit(2)){
	    int i = 0;
	    for(i = 0; i <  sizeof(eth_Ipaddr)/ sizeof(int); i++){
		if(this_packet_src_ip + 100 == eth_Ipaddr[i]){
			if(ntohs(iph->tot_len) > 1000 && des_port == 5201)
                        b_data[i]++;
		}
	    }
	}
/*
	    switch(this_packet_src_ip) //check inbound packet staticstis 
	    {
	         case 2:
        	    if(ntohs(iph->tot_len) > 1000 && des_port == 5201)
        	        b_data[1]++;
		    break;
	     
                 case 3:
                    if(ntohs(iph->tot_len) > 1000 && des_port == 5201)
                        b_data[2]++;
                    break;

                 case 4:
                    if(ntohs(iph->tot_len) > 1000 && des_port == 5201)
                        b_data[3]++;
                    break;

                 case 5:
                    if(ntohs(iph->tot_len) > 1000 && des_port == 5201)
                        b_data[4]++;
                    break;

       	         default: //Some Other IP Addresses
                    break;
	    }
	}
	else{
            switch(this_packet_des_ip) //check outbound packet staticstis 
            {
                 case 2:
                    if(ntohs(iph->tot_len) > 1000 && src_port == 13562)
                        local_data[1]++;
                    break; 
             
                 case 3:
                    if(ntohs(iph->tot_len) > 1000 && src_port == 13562)
                        local_data[2]++;
                    break;

                 case 4:
                    if(ntohs(iph->tot_len) > 1000 && src_port == 13562)
                        local_data[3]++;
                    break;

                 case 5:
                    if(ntohs(iph->tot_len) > 1000 && src_port == 13562)
                        local_data[4]++;
                    break;

                 default: //Some Other IP Addresses
                    break;
           }
	} 
*/
    }
    if(zclock_mono () >= expiry){
       	//construct data to send
        char str_tosend[200];
	memset(str_tosend, 0, 200*sizeof(char)); 
	char c_num [5];
  //      printf("Captured:");
	int i = 0; 
    	for (i = 0 ; i < 9 ;i++) {
	    sprintf(c_num,"%d",b_data[i]);
            strcat(str_tosend,c_num);
	    memset(c_num, 0, sizeof (c_num)); 
	    if(i < 8)
	        strcat(str_tosend,".");
	    else 
		strcat(str_tosend,"\0");
//            printf("%d ",b_data[i]);
        }     
//	printf("\n");
//	printf("str_tosend:%s\n",str_tosend);
	expiry = zclock_mono () + 1000;
	i=0;
	int sum = 0;
	for (i = 0; i < sizeof(b_data)/sizeof(int); i++)
		sum += b_data[i];
	if(sum <= 0 ){
	    memset(str_tosend, 0, sizeof (str_tosend));
//	    memset(local_data, 0, sizeof (local_data));
//	    memset(from_others, 0, sizeof (from_others));
   	    return;
	}
//	printf("str_tosend:%s\n",str_tosend);
	zsock_send (speaker, "ssi", "PUBLISH",str_tosend, sizeof(tosend), 10);
	memset(str_tosend, 0, sizeof (str_tosend));
//	memset(from_others, 0, sizeof (from_others));
//	memset(local_data, 0, sizeof (local_data));
        memset(b_data, 0, sizeof (b_data)); 
	usleep(100);
    }
}
 

