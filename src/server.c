#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define error(res,s,escape) do{\
	if(res == -1){perror(#s);goto escape;}\
}while(0)

#define judge(exp) do{\
	(exp)?(printf("%s: true\n", #exp)):(printf("%s: false\n", #exp));\
}while(0)

#define PORT 5555

void server(int sockfd);
void my_exec(char *cmd);

int main(int argc, char *argv[])/*{{{*/
{
	int  sockfd;
	int res;
	int port = PORT;

	if(argc > 1)
		port = atoi(argv[1]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	error(sockfd, socket,_main_end);

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);
	
	res = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
	error(res, bind, _main_end);	
	res = listen(sockfd, 5);
	
	error(res, listen, _main_end);	
	struct sockaddr_in client_address;
	memset(&client_address, 0, sizeof(client_address));
	int new_sockfd;
	int address_len = sizeof(client_address);
	int pid;
	while(1)
	{
		printf("accepting.....\n");
		new_sockfd = accept(sockfd, (struct sockaddr *)&client_address, &address_len);	
		error(new_sockfd, accept, _main_end);
		if(new_sockfd != -1)
		{
			pid = fork();
			if(pid > 0)
				close(new_sockfd);
			else
				server(new_sockfd);		
		}
	}
_main_end:
	close(sockfd);
	return argc;
}/*}}}*/

void server(int sockfd)/*{{{*/
{
	int res;
	char buf[10000];
	fd_set rset;
	fd_set wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(sockfd, &rset);
	FD_SET(sockfd, &wset);

	res = select(sockfd + 1, NULL, &wset, NULL, NULL);
	error(res, write, _server_end);
	res = write(sockfd, "server is ready\n",16);
	error(res, write, _server_end);
	
	struct sockaddr_in client_address;
	int  address_len = sizeof(client_address);
	res = getpeername(sockfd, (struct sockaddr *)&client_address, &address_len);
	
	char *ip = inet_ntoa(client_address.sin_addr);
	int port = ntohs(client_address.sin_port);

	printf("client:%s#%d connect success\n", ip, port);

	memset((void *)buf, 0, 10000);
	while(1)
	{
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(sockfd, &rset);
		FD_SET(sockfd, &wset);

		res = select(sockfd + 1, &rset, NULL, NULL, NULL);
		error(res, select, _server_end);
		res = read(sockfd, buf,10000);
		error(res, read, _server_end);

		if(res == 0)
			goto _server_end;

		printf("%s#%d: %s\n", ip, port, buf);

		int out, err;
		out = dup(1);
		err = dup(2);
		dup2(sockfd, 1);
		dup2(sockfd, 2);
		my_exec(buf);
		write(sockfd, ">", 2);
		dup2(out,1);
		dup2(err,2);
		
		memset((void *)buf, 0, 10000);
	}
_server_end:
	printf("client:%s#%d disconnect!\n", ip, port);	
	close(sockfd);
	exit(0);
}/*}}}*/

void my_exec(char *cmd)/*{{{*/
{
	int res;
	res = system(cmd);
	error(res, system, _my_exec);
_my_exec:
	return ;
	
}/*}}}*/
