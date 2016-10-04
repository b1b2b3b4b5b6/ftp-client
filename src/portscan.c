#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
typedef struct Iport_ 
{
	char *ip;
	int start_port;
	int end_port;
}Iport;

int sayn_conn(const char *ip, int port);
void *scan_ports(Iport *iport); 
void multi_scan(Iport *iport, int num_thread);

int main(int argc, char *argv[])
{
	struct timeval tpstart, tpend;
	Iport iport;
	int num_thread;
	if(argc < 6)
		printf("param error\n");
	
	iport.ip = argv[1];
	iport.start_port = atoi(argv[2]);
	iport.end_port = atoi(argv[3]);
	if(argv[4] != NULL)
	{
		num_thread = atoi(argv[4]);
	}
	else
		num_thread = 1; 
	
	gettimeofday(&tpstart, NULL);
	printf("scan ip :%s\n", iport.ip);
	printf("port:%d ~ %d\n", iport.start_port, iport.end_port);
	printf("count of pthread_t is %d\n",num_thread);
	multi_scan(&iport, num_thread);
	gettimeofday(&tpend, NULL);
	int time = tpend.tv_sec - tpstart.tv_sec;
	printf("used time is %d\n", time);
	return argc;
}

int sayn_conn(const char *ip, int port)
{
	int errno;
	fd_set rset, wset;
	FD_ZERO(&wset);
	FD_ZERO(&rset);
	struct timeval time;
	time.tv_sec = 2;
	time.tv_usec = 0;	

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(port);

	int res = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
	if((res != 0) && (errno != EINPROGRESS))
	{
		perror("connecting error");
		close(sockfd);
		return -1;
	}
	else
	{
		close(sockfd);
		return 0;	
	}

	FD_SET(sockfd, &rset);
	FD_SET(sockfd, &wset);

	res = select(sockfd + 1, &rset, &wset, NULL, &time);

	switch(res)
	{
		case -1:
			perror("select error");
		break;

		case 0:
		break;	

		default:
			if(FD_ISSET(sockfd,&rset) || FD_ISSET(sockfd, &wset))
			{
				socklen_t error = -1;
				socklen_t len = sizeof(error);
				if(getsockopt(sockfd,SOL_SOCKET, SO_ERROR, &error, &len) < 0)
				{
					perror("getsockopt fail");
				}


				if(error == 0)
				{
					close(sockfd);
					return 0;
				}



			}
	}
	close(sockfd);
	return -1;
}

void *scan_ports(Iport *iport)
{
	int n;

	for(n = iport->start_port; n <= iport->end_port; n++)
	{
		if(sayn_conn(iport->ip, n) == 0)
			printf("port:%d is open\n", n);
		else
			printf("port:%d is close\n", n);
			
	}
	return (void *)iport;
}


void multi_scan(Iport *iport, int thread_count)
{
	int start_port = iport->start_port;
	int end_port = iport->end_port;
	int port_count = end_port - start_port + 1;
	char *ip = iport->ip;
	int section = port_count/thread_count;
	if(section == 0)
	{
		scan_ports(iport);	
		return;
	}
	if((port_count % thread_count) != 0)
		thread_count++;
	
	Iport *temp;
	pthread_t *id;
	
	id = malloc(thread_count * sizeof(pthread_t));

	int n;
	int offset;
	int count;
	for(n = 0 ;n < thread_count; n++)	
	{	
		offset = section * n;
		count = (port_count - offset) < section ? (port_count - offset) : section;
		temp = (Iport *)malloc(sizeof(Iport));
		temp->ip = ip;
		temp->start_port = start_port + offset; 
		temp->end_port = temp->start_port + count -1;

		pthread_create(id, NULL, (void *(*)(void *))scan_ports, (void *)temp);
		id++;
	}
	id--;
	for(n = 0; n < thread_count; n++)
	{
		pthread_join(*id, (void **)&temp);
		free(temp);
		id--;
	}
	id++;
	free(id);

	return ;
}
