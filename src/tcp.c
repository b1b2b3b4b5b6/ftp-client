#include "tcp.h"

int tcp_connect(char *ip, char *port)
{	
	int res;
	int errno;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);;
	 
	 
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(atoi(port));

	res = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

	if((res != 0) && (errno != EINPROGRESS))
	{
		error(-1, connect_error, goto end);
	}
	 
	fd_set rset,wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(sockfd, &rset);
	FD_SET(sockfd, &wset);
	struct timeval time;
	time.tv_sec = 2;
	time.tv_usec = 0;
	res = select(sockfd + 1, &rset, &wset, NULL, &time);
	switch(res)
	{
		case -1:
			sys_error(-1, select error, goto end);
		break;

		case 0:
		break;

		default:
			if(FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
			{
				int err = -1;
				socklen_t len = sizeof(err);
				res = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
				sys_error(res, getsockopt, goto end);
				if(err == 0)
				{
					fcntl(sockfd, F_SETFL, flags);
					return sockfd;	
				}
			}
		break;
	}

end:
	close(sockfd);
	return -1;	
}

