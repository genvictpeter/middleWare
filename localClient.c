#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
 
 
#define LOCAL_PORT 8888
#define REMOTW_PORT 9999
 
int main(void)
{
    int client_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (client_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
	
	struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(LOCAL_PORT);
	
	
	if (bind(client_sock,(struct sockaddr*)&local_addr,sizeof(local_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
	int optval = 1;
	setsockopt(client_sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
 
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(REMOTW_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
 
    socklen_t ser_len = sizeof(serv_addr);
	socklen_t local_len = sizeof(local_addr);
    char send_buff[1024] = {0};
    char recv_buff[1024] = {0};
 
    while (true) {
		printf("start recv data\n");
        int ret = recvfrom(client_sock,recv_buff,255,0,
                (struct sockaddr*)&local_addr,&local_len);
        printf("recv ret = %d\n", ret);
        if (ret > 0) {
            recv_buff[ret] = 0;
            printf("recv %d bytes:%s from %s:%d\n",ret,recv_buff,
                inet_ntoa(local_addr.sin_addr),ntohs(local_addr.sin_port));
        }
		
		//sendto(client_sock, recv_buff, ret, 0, (struct sockaddr*)&serv_addr,ser_len);
    }
 
    return 0;    
}