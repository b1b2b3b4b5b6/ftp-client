#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define error(res, s) do{ \
	if(res == -1) perror(#s); \
}while(0) 

#define judge(exp) do{\
	(exp)?(printf("%s: true\n", #exp)):(printf("%s: false\n", #exp));\
}while(0)

#define PORT 5555
#define IP	"127.0.0.1"


void receive(int sockfd)
{
	char buf[10000];
	memset((void *)buf, 0, 1000);
	fd_set rset;
	int res;
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	struct sockaddr_in client_address;
	int  address_len = sizeof(client_address);
	res = getsockname(sockfd, (struct sockaddr *)&client_address, &address_len);
	
	char *ip = inet_ntoa(client_address.sin_addr);
	int port = ntohs(client_address.sin_port);
	printf("local port: %d\n", port);
	while(1)
	{
		
		res = select(sockfd + 1, &rset, NULL, NULL, NULL);
		error(res, select);
		res = read(sockfd, buf, 10000);
		error(res, read);
		if(res == 0)
			{
				printf("server disconnect!\n");
				close(sockfd);
				return ;
			}
		printf("%s", buf);	
		fflush(stdout);
		memset((void *)buf, 0, 10000);
	}
}

void send_cmd(int sockfd)
{
	char buf[10000];
	memset((void *)buf, 0, 10000);
	int len;
	while(1)
	{
		gets(buf);
		len = strlen(buf);
		write(sockfd, buf, len + 1);
	}
}

int main(int argc, char *argv[])/*{{{*/
{
	int sockfd;
	int res;
	char *ip = IP;
	int port = PORT;
	
	if(argc > 1)
		ip = argv[1];
	if(argc > 2)
		port = atoi(argv[2]);
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));	

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	error(sockfd, socket);
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip);
	server_address.sin_port = htons(port);

	printf("connecting......\n");
	res = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
	error(res, connect);	
	printf("server:%s#%d connect success\n", ip, port);

	int pid;
	pid = fork();
	if(pid > 0)
	{
		receive(sockfd);
		kill(pid, SIGKILL);
		wait(NULL);
	}
	else
		send_cmd(sockfd);
	return argc;
}/*}}}*/

