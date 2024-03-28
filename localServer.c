#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
 
#define LOCAL_PORT 9999
#define REMOTW_PORT 8888
 
int main(void)
{
    int serv_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (serv_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
 
    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(LOCAL_PORT);
 
    if (bind(serv_sock,(struct sockaddr*)&local_addr,sizeof(local_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
 
    char message[1024] = {0};
	
	int optval = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
	struct sockaddr_in remote_addr = {0};
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	remote_addr.sin_port = htons(REMOTW_PORT);
	
	socklen_t remote_len = sizeof(remote_addr);
	socklen_t local_len = sizeof(local_addr);
	 char recv_buff[1024] = {0};
	
    while (true) {
		
        sendto(serv_sock,message,sizeof(message),
            0,(struct sockaddr*)&remote_addr,remote_len
        );
		
		/*int ret = recvfrom(serv_sock, message, sizeof(message), 0, (struct sockaddr*)&local_addr, &local_len);
		if(ret > 0) {
			recv_buff[ret] = 0;
            printf("recv %d bytes:%s from %s:%d\n",ret,recv_buff,
                inet_ntoa(local_addr.sin_addr),ntohs(local_addr.sin_port));
		}*/
		sleep(1);
    }
 
    return 0;
}  