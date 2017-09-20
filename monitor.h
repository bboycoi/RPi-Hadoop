#include "zhelpers.h"
#include "czmq.h"
#include <net/if.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//  This is our client task
//  It connects to the server, and then sends a request once per second
//  It collects responses as they arrive, and it prints them out. We will
//  run several client tasks in parallel, each with a different random ID.

#define HEARTBEAT     1000       	//  In msecs

pthread_mutex_t qu_mutex2 = PTHREAD_MUTEX_INITIALIZER;

int reduce = 0;
int fast_update = 0;
extern int data_in_q;   
int slot=0;

struct mapper_t {
   char  output_size[10];
   char  map_id[39];
   char  job_id[19];
}mapper;

void initialize_mapper(struct mapper_t *map){
//	map->output_size = malloc(sizeof(byte));
//	map->map_id = malloc(sizeof(byte));
//	map->job_id = malloc(sizeof(byte));
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

int file_is_modified(const char *path) {
    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err != 0) {
        perror(" [file_is_modified] stat");
        exit(errno);
    }
    return file_stat.st_mtime;
}

int* get_link_quality(){
	static int speed[9];
	FILE *fp = fopen("speed_log.txt","r");
        if (fp==NULL)
        	perror ("Error reading file");
	const char s[2] = ".";
        char *token;
	char line[384];
	if(fgets ( line, sizeof line, fp ) != NULL ){
        	token = strtok(line, s);// We skip node1
		int i = 1;
		for(i = 1; i < 9; i++){
			if(token != NULL){
				token = strtok(NULL,s);
				if(token != NULL)    
					speed[i] = atoi(token);  
			}
		}
	}
	return speed;
}

int map_list_cmp(zlist_t *list, struct mapper_t *map){
	int cmp = 0;
//	zlist_first (list);
	struct mapper_t *next_map = zlist_first(list);
	while(next_map != NULL){
//		printf("next map ID:%s\n",next_map->map_id);
//		printf("new map ID:%s\n",map->map_id);
		if(strcmp(next_map->map_id, map->map_id) == 0)
			return 1;
		next_map = zlist_next(list);
	}
	return cmp;
}

char * int_arry_toa(int p[]){
	char *str = malloc(100*sizeof(char));
	char c_num [5];
        int k = 0;
	memset(str, '\0', 100*sizeof(char)); 
        for (k = 0 ; k < 9 ;k++) {
        	sprintf(c_num,"%d",p[k]);
           	strcat(str,c_num);
            	memset(c_num, 0, sizeof (c_num)); 
                if(k < 8)
                	strcat(str,".");
            	else 
                        strcat(str,"\0");
//  		printf("%s",str);
      }
//	printf("\n");
//	printf("%s\n",str);	
	return str;
}


int get_data_size(zlist_t *list){
	int size = 0;
	struct mapper_t *map = zlist_first(list);
        while(map != NULL){
		size += atoi(map->output_size);
		map = zlist_next(list);
        }
        return size;
}

//Listien to broadcast message
void *recv_broadcast(void *listener){
   	 while(1){
        	char *ipaddress = zstr_recv (listener);
        	if (ipaddress){
//          		char *content = zstr_recv (listener);	
//          		zstr_free(&content);
			pthread_mutex_lock(&qu_mutex2);
			fast_update = 1;
			pthread_mutex_unlock(&qu_mutex2);
        		zstr_free(&ipaddress);
        	}
		else zstr_free(&ipaddress);
	}
}


//  This is our client task
void * report_task (){
	FILE *fp = fopen("mapsToSend.txt","r");
        if (fp==NULL){                          
        	printf ("File mapsToSend.txt isn't existed, creating new file\n");
		fp = fopen("mapsToSend.txt","w");
		fclose(fp);
		chmod("mapsToSend.txt", S_IRWXU|S_IRGRP|S_IXGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IXOTH);
        }

	pthread_t pt;
    	// Set broadcast interface to ETH0
    	zsys_set_interface ("eth0");
    	printf("\n");

    	//initiate listener
	zactor_t *listener = zactor_new (zbeacon, NULL);
    	assert (listener);
   	// zstr_sendx (listener, "VERBOSE", NULL);
    	zsock_send (listener, "si", "CONFIGURE", 2018);
    	char* hostname = zstr_recv (listener);
    	assert (*hostname);
    	free (hostname);
	
	//  We will listen to anything (empty subscription)
    	zsock_send (listener, "sb", "SUBSCRIBE", "", 0);
    	if (pthread_create(&pt, NULL,recv_broadcast, listener) != 0){
        	perror("Create Thread");
        	exit(1); 
    	}
	zsock_set_rcvtimeo (listener, 500);
	int n = get_Last_IP_Digit();	//  Create a DEALER socket. Default action is connect.	
	char identity [6];
	const char * path = "mapsToSend.txt";
	long data_size = 0;
	int64_t expiry = zclock_mono () + HEARTBEAT;
	zlist_t *pre_map_list = zlist_new ();
	zlist_t *post_map_list = zlist_new ();
//	void *context = zmq_ctx_new();
//	void *client = zmq_socket(context, ZMQ_DEALER);
	zsock_t *client = zsock_new_dealer("tcp://10.0.0.101:5671");
	zsock_set_sndtimeo(client,500);
	sprintf (identity,"node%d",n); 	//  Set identity to make tracing easier
//	zmq_connect (client, "tcp://10.0.0.101:5671");
	//  Loop sends update every 2 seconds
	int read = 0;
	while (1){
		pthread_mutex_lock(&qu_mutex2);
		char line[384];
		fp = fopen("mapsToSend.txt","r");
     		if (fp==NULL){
      			perror ("Error reading file");
		}
//		zfile_t *fp = zfile_new (NULL, "test.txt");
		int i = 0;
		while (fgets ( line, sizeof line, fp ) != NULL ){  //  read a line in file
//		const char *const_line = zfile_readln (fp);
//		while(const_line != NULL){
//			char *line = strdup(const_line);
			i++;
			//initialize_mapper(map);
			if(i > read){
				const char s[2] = ".";
				char *token;
				token = strtok(line, s);
				if(token != NULL){
//					printf("%s\n",token);
					int mess_type = atoi(token);
//					printf("mess type %d\n",mess_type);
					if(mess_type == 3){
						token = strtok(NULL,s);//skip message
						token = strtok(NULL,s);
						char sub_token[5];
						memcpy(sub_token,&token[22],4);
						sub_token[5] = '\0';
						reduce = atoi(sub_token);
//						slot = 0;
						continue;
					}else
                                        if(mess_type == 4){
                                                reduce = 0;
						printf("job done, read == %d\n",read);
                                                break;
                                        }
     					token = strtok(NULL,s);	//  job ID and attemptID
//					struct mapper_t *cur1 = zlist_first(pre_map_list);
//                                        if(cur1 != NULL) 
//                                                printf("cursor1:%s\n", cur1->map_id);
//					printf("%s\n",token);
//					map->map_id = malloc(strlen(token)+sizeof(char));
//					memcpy(map->map_id,token,strlen(token));

					struct mapper_t *map = malloc(sizeof(struct mapper_t));
					sprintf(map->map_id,"%s\0",token);
//					printf("map id:%s\n",map->map_id);
//					map->job_id = malloc(19*sizeof(char));
					memcpy(map->job_id,&map->map_id[8],18);
					map->job_id[18] = '\0';
//					printf("job id:%s\n",map->job_id);
					token = strtok(NULL, s);	//  data size
//					map->output_size = malloc(strlen(token)+sizeof(char));
					sprintf(map->output_size,"%s\0",token);
//					printf("Size:%s\n",map->output_size);
		        		token = strtok(NULL,s);	// skip the timestamp
					token = strtok(NULL,s);	// next value depends on the type of message
//					printf("Pre map size:%d\n",zlist_size(pre_map_list));
					switch (mess_type)
					{
						case 1: 
							if(map_list_cmp(pre_map_list,map) == 0){
								zlist_append(pre_map_list,map);
//								printf("case1 Pre map size:%d\n",zlist_size(pre_map_list));
								zlist_first (pre_map_list);
							}
							break;

						case 0:
							if(map_list_cmp(pre_map_list,map) == 1){
								struct mapper_t *cur = zlist_item(pre_map_list); 
//								printf("cursor:%s\n", cur->map_id);
//								free(cur->map_id);
//								free(cur->job_id);
//								free(cur->output_size);
								zlist_remove(pre_map_list,cur);
								free(cur);
								zlist_first (pre_map_list);
//								if(zlist_size(pre_map_list) == 0 && reduce == 0){
								if(reduce == 0){
                                                                        slot = 0;
									int *p = get_link_quality();
					                        	int size = 0;
                        					//	if(reduce == 0)
								//	if (data_in_q > 0)
                                						size = get_data_size(pre_map_list); 
                        						char *buff = malloc(sizeof(char) * 200);
                        						char *str = int_arry_toa(p);
                        						sprintf (buff,"%d.%d.%d.%s",n,size,reduce,str); 
                        						zstr_sendf (client, "%s",buff);
                        						free(buff);
                        						free(str);
                                                                }
//								printf("case2 Pre map size:%d\n",zlist_size(pre_map_list));
								zlist_append(post_map_list,map);
//								printf("case2 Post map size:%d\n",zlist_size(post_map_list));
							}
							break;

						case 2:
							if(map_list_cmp(pre_map_list,map)){
								struct mapper_t *cur = zlist_item(pre_map_list); 
//                                                                printf("cursor:%s\n", cur->map_id);
//                                                              free(cur->map_id);
//                                                              free(cur->job_id);
//                                                              free(cur->output_size);
                                                                zlist_remove(pre_map_list,cur);
								free(cur);
								if(zlist_size(pre_map_list) == 0 && reduce == 0){
									slot = 0;
                                                                        int *p = get_link_quality();
							                int size = 0;
                        					//	if(reduce == 0)
								//	if (data_in_q  > 0)  
                                						size = get_data_size(pre_map_list); 
                                                                        char *buff = malloc(sizeof(char) * 200);
                                                                        char *str = int_arry_toa(p);
                                                                        sprintf (buff,"%d.%d.%d.%s",n,size,reduce,str); 
                                                                        zstr_sendf (client, "%s",buff);
                                                                        free(buff);
                                                                        free(str);
								}
 //								printf("case3 Pre map size:%d\n",zlist_size(pre_map_list));
								//TODO STOP TRANFER (MAYBE) AND INFORM MASTER
							}
							break;

						default:
							break;
					}
					memset( line, '\0', sizeof(line));
//					const_line = zfile_readln (fp);
				}
			}
		}
		read = i;
		fclose(fp);
//		zfile_close (fp);
		pthread_mutex_unlock(&qu_mutex2);
//		if(zclock_mono() >= expiry || fast_update !=0 ){
		if(fast_update !=0 ){
			int *p = get_link_quality();
			int size = 0;
		//	if(reduce == 0)
		//	if (data_in_q  > 0)  
				size = get_data_size(pre_map_list); 
			char *buff = malloc(sizeof(char) * 200);
			char *str = int_arry_toa(p);
			sprintf (buff,"%d.%d.%d.%s",n,size,reduce,str); 
//			printf("%s\n",buff);
			zstr_sendf (client, "%s",buff);
			free(buff);
			free(str);
//			data_size = 0; 
                        pthread_mutex_lock(&qu_mutex2);
                        fast_update = 0;
                        pthread_mutex_unlock(&qu_mutex2);
//			expiry = zclock_mono () + HEARTBEAT;
		}
		zclock_sleep (20);
	}
	zlist_destroy (&pre_map_list);
	zlist_destroy (&post_map_list);
        zsock_destroy (&client);
//    	zmq_close(client);
	zactor_destroy (&listener);
//	zmq_ctx_destroy(context);
}


//  This is our server task
void * response_task (void *args){
	//  Create a ROUTER socket. Default action is connect.  
	int p[6] = {0,0,0,0,0};
	void *context = zmq_ctx_new();
	void *server = zmq_socket(context, ZMQ_ROUTER);
	zmq_bind(server, "tcp://*:5671"); /**/
	while(1){
		char *content = zstr_recv (server);
		printf("%s\n",content);
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
			token = strtok(NULL,s); // is reducer
			if (atoi(token) == 1)
				printf("is reducer\n");
			token = strtok(NULL,s);   // speed rate packet per second
			while(token != NULL){   
				int tk = atoi(token);
				speed += tk;
				if(tk!=0)
					count++;
				token = strtok(NULL,s);
			}
			if(count != 0)
				speed = speed/count;
			int i = 0;
			for(i = 1; i < sizeof(p)/sizeof(int);i++ ){
				if (i == nodeId){
					p[i] += size;
				}
			} 
		}
		printf("node%d, size:%d, speed:%d\n", nodeId,p[nodeId],speed);
		printf("-----------------------------------------------------------\n");

		free(content);
	}
    	zmq_close(server);
	zmq_ctx_destroy(context);
}


// CHECK FOR REDUCER (NOT USED ANYMORE)
void *folder_watch(){
	zactor_t *watch = zactor_new (zdir_watch, NULL);
    	assert (watch);
	char root_path [] = "/hdfs/tmp/nm-local-dir/usercache/";
//	char root_path [] = "./test_dir";
	FILE *fp = fopen("test.txt","w");
	fclose(fp);
//	zdir_t *older = zdir_new ("./test_dir",NULL); 	///hdfs/tmp/nm-local-dir/usercache/hd/appcache/", NULL);
	zdir_t *older = zdir_new ("/hdfs/tmp/nm-local-dir/usercache/", NULL);  
  	assert (older);
    	if (1) {
        	printf ("\n");
        	zdir_dump (older, 0);
    	}
    	zdir_t *newer = zdir_new (".", NULL);
    	assert (newer);
    	zlist_t *patches = zdir_diff (older, newer, "/");
    	assert (patches);

    	int synced;
    	if (1) {
        	zsock_send (watch, "s", "VERBOSE");
        	synced = zsock_wait(watch);
        	assert ( synced == 0);
    	}
//	zsock_send (watch, "ss", "SUBSCRIBE", "./test_dir");	///hdfs/tmp/nm-local-dir/usercache/hd/appcache/");
	zsock_send (watch, "ss", "SUBSCRIBE","/hdfs/tmp/nm-local-dir/usercache/");
	while (1){
    		synced = zsock_wait(watch);
    	//	assert(synced == 0);
		char *path;
    		int rc = zsock_recv (watch, "sp", &path, &patches);
    		assert (rc == 0);
		free(path);
  		if (zlist_size (patches) > 0){
    			int i = 0;
			int nmbs_file = zlist_size (patches);
			for (i = 0; i < nmbs_file; i++){
				zdir_patch_t *patch = (zdir_patch_t *) zlist_pop (patches);
    				zfile_t *patch_file = zdir_patch_file (patch);
				zfile_restat(patch_file);
				if (strstr(zfile_filename (patch_file,root_path),"_r_0000")){
					reduce = 1;
					zdir_patch_destroy (&patch);
					printf("Is reduce\n");
//					zsock_send (watch, "ss", "UNSUBSCRIBE", "/hdfs/tmp/nm-local-dir/usercache/hd/appcache/");
//					break;
				}
				zdir_patch_destroy (&patch);
			}
		}
//		zsock_send (watch, "si", "TIMEOUT", 100);
		zsock_send (watch, "s", "VERBOSE");
	}
	zlist_destroy (&patches);
	zactor_destroy (&watch);
	return;
}
