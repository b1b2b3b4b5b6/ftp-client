#include "common.h"
#include "tcp.h"

#define DEBUG 1
#define CMD_BUF 100 

#define ftp_cmd(sockfd, s, ...) ({\
	char *buf = malloc(CMD_BUF);\
	memset(buf, 0, CMD_BUF);\
	sprintf(buf, s, __VA_ARGS__);\
	int len = strlen(buf);\
	res = write(sockfd, buf, len);\
	sys_error(res, write, hander());\
	if(DEBUG) printf("client: %s",buf);\
	free(buf);\
	res;\
})

#define ftp_read(sockfd, buf, len) ({\
	int nread = 0;\
	if(wait_read(sockfd,1) == 0)\
		nread = read(sockfd, buf, len);\
	nread;\
})
int sock;

enum  ftp_cmd{
	LS = 1,
	PWD,
	MDTM,
	DOWNLOAD,
	UPLOAD,
	REMOVE,
	FSIZE,
	SCAN,
	CAT
};

enum ftp_type{
	vsFTPd = 1,
	Microsoft,
};

typedef struct _ftp_t 
{
	int sockfd;
	int code;
	enum ftp_type type;
	enum ftp_cmd cmd;
	char dataserver[30];
	char dataport[10];
	int logged;
	char user[100];
	char password[100];
	char server[30];
	char port[10];
	char remotepath[256];
	char localpath[256];
}ftp_t;

void ftp_init(ftp_t *ftp)
{
	memset(ftp, 0 ,sizeof(ftp_t));
	strcpy(ftp->server, "127.0.0.1");
	//strcpy(ftp->server, "192.168.10.200");
	strcpy(ftp->user, "ftpuser");
	//strcpy(ftp->user, "god");
	strcpy(ftp->password, "topeet");
	//strcpy(ftp->password, "Aa201020");
	strcpy(ftp->port, "21");
	strcpy(ftp->remotepath, "./");
	strcpy(ftp->localpath, "./");
	ftp->cmd = UPLOAD;
}
void hander()
{
	exit(-1);
}


int wait_read(int fd, int sec)
{
	int res;
	struct timeval time;
	time.tv_sec = sec;
	time.tv_usec = 0;
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	res = select(fd + 1, &rset, NULL, NULL, &time);
	sys_error(res, select, hander());
	if(res > 0)
		return 0;
	else
		return -1;
}

int ftp_getline(ftp_t *ftp)
{
	int res;
	char line[100];
	memset(line, 0, sizeof(line));
	int code;
	char *s;
	ftp->code = 0;
	
	res = wait_read(ftp->sockfd, 1);
	error(res, NULL, return -1);
	res = read(ftp->sockfd, line, sizeof(line));
	sys_error(res, read, hander());

	#if DEBUG
		printf("server: %s", line);
	#endif

	code = strtol(line, &s, 10);
	if((code != 0) && (s != NULL))
	{
		ftp->code = code;
		switch(code)
		{
			case 227:
			{
				int h1, h2, h3, h4, p1, p2;
				char *iport = strchr(s, '(');
				
				res = sscanf(iport, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
				sprintf(ftp->dataserver, "%d.%d.%d.%d", h1, h2, h3, h4);
				sprintf(ftp->dataport, "%d", (256*p1 + p2));

				#if DEBUG 
					printf("data server ip: %s\n", ftp->dataserver);
					printf("data port: %s\n", ftp->dataport);
				#endif
			}
			break;
			case 220:
			{
				if(strpbrk(s, "vsFTPd") == 0)
					ftp->type = vsFTPd;
				if(strpbrk(s, "Microsoft") == 0)
					ftp->type = Microsoft;

			}
			break;
		}
		return 0;
	}
	return -1;
}
 
int ftp_banner(ftp_t *ftp)
{	
	int res;
	res = ftp_getline(ftp);
	return res;
}
 
int ftp_login(ftp_t *ftp)
{
	int res;
	ftp_cmd(ftp->sockfd, "USER %s\r\n", ftp->user);
	res = ftp_getline(ftp);
	error(res, ftp_getline error, return -1);

	if((ftp->code == 220) || (ftp->code == 230))
	{
		ftp->logged = 1;
		return 0;
	}

	ftp_cmd(ftp->sockfd, "PASS %s\r\n", ftp->password);
	res = ftp_getline(ftp);
	error(res, ftp_getline error, return -1);

	if((ftp->code == 220) || (ftp->code == 230))
	{
		ftp->logged = 1;
		return 0;
	}
	
	return -1;
}

int ftp_mkcon(ftp_t *ftp)
{
	int res;
	ftp->sockfd = tcp_connect(ftp->server, ftp->port);
	error(ftp->sockfd, cannot  connect to server, return -1);
	res = ftp_banner(ftp);
	error(res, ftp_banner error, return -1);
	res = ftp_login(ftp);
	error(res, ftp_login error, return -1);
	return 0;
}

int ftp_getdataport(ftp_t *ftp)
{
	int res;
	ftp_cmd(ftp->sockfd, "PASV\r\n", NULL);
	res = ftp_getline(ftp);
	error(res, ftp_getline error, return -1);

	return 0;
}


int ftp_list(ftp_t *ftp)
{
	int res;
	res = ftp_getdataport(ftp);
	error(res, ftp_getdataport, return -1);
	int sockfd = tcp_connect(ftp->dataserver, ftp->dataport);
	error(sockfd, tcp_connect error, return -1);
	printf("data server connect success\n");

	ftp_cmd(ftp->sockfd, "NLST %s\r\n", ftp->remotepath);
	res = ftp_getline(ftp);
	error(res, no response, );

	char buf[1024];
	memset(buf, 0, sizeof(buf));
	int nread;
	while(1)
	{
		nread = ftp_read(sockfd, buf, sizeof(buf) - 1);
		if(nread == 0)	
			break;
		printf("%s", buf);
	}
	int len = strlen(buf);
	printf("datatransport finish\n");
	close(sockfd);
	return 0;
}


int ftp_download(ftp_t *ftp)
{
	int res;
	res = ftp_getdataport(ftp);
	error(res, ftp_getdataport error, return -1);
	ftp_cmd(ftp->sockfd, "TYPE I\r\n", NULL);
	res = ftp_getline(ftp);

	int sockfd = tcp_connect(ftp->dataserver, ftp->dataport);
	error(sockfd, tcp_connect error return -1, return -1);
	printf("data server connect success\n");
	ftp_cmd(ftp->sockfd, "RETR %s\r\n", ftp->remotepath);
	res = ftp_getline(ftp);
	error(res, no response, );
	char buf[4096];	
	int nread;
	int fd = open(ftp->localpath, O_RDWR|O_CREAT);
	if(fd == -1)
	{
		printf("can't open %s\n",ftp->localpath);
		perror("error");
		close(sockfd);
		return -1;
	}

	while(1)
	{
		nread = ftp_read(sockfd, buf, sizeof(buf));
		if(nread == 0)
			break;
		write(fd, buf, nread);
	}
	printf("download finished\n");
	close(fd);
	close(sockfd);

	return 0;
}

int ftp_upload(ftp_t *ftp)
{
	int res;
	int sockfd;
	res = ftp_getdataport(ftp);
	error(res, ftp_getdataport error, return -1);
	ftp_cmd(ftp->sockfd, "TYPE I\r\n", NULL);
	res = ftp_getline(ftp);
	if(ftp->type = Microsoft)
	{
		sockfd = tcp_connect(ftp->dataserver, ftp->dataport);
		error(sockfd, tcp_connect error, return -1);
		
	}
	printf("data server connect success\n");
	ftp_cmd(ftp->sockfd, "STOR %s\r\n", ftp->remotepath);
	res = ftp_getline(ftp);
	if(ftp->type == vsFTPd)
	{
		sockfd = tcp_connect(ftp->dataserver, ftp->dataport);
		error(sockfd, tcp_connect error, return -1);
	}

	if(ftp->code != 150)
	{
		printf("send faild, server refused\n");
		close(sockfd);
		return -1;
	}

	char buf[4096];
	int nread;
	int fd = open(ftp->localpath, O_RDONLY);
	if(fd == -1)
	{
		printf("can't open %s\n", ftp->localpath);
		perror("error");
		close(sockfd);
		return -1;
	}
	while(1)
	{
		nread = read(fd, buf, sizeof(buf));	
		if(nread == 0)
			break;
		write(sockfd, buf, nread);
	}
	printf("upload finished\n");
}

int ftp_usage()
{
	printf("OPTIONS:\n"
			"	--server -s : FTP server host.\n"
			"	--user	 -u : FTP username.\n"
			"	--pass	 -p : FTP password.\n");
	exit(-1);
}

int param_parse(ftp_t *ftp, int argc, char *argv[])
{
	int opt;
	static struct option longopts[] = {
		{"server",	required_argument,	0,	's'},
		{"user",	required_argument,	0,	'u'},
		{"pass",	required_argument,	0,	'p'}, };
	while((opt = getopt_long(argc, argv, "s:u:p:", longopts, NULL)) != -1)
	{
		switch(opt)
		{
			case 's':
				strcpy(ftp->server, optarg);
			break;
			
			case 'u':
				strcpy(ftp->user, optarg);
			break;

			case 'p':
				strcpy(ftp->password, optarg);
			break;
			
			default :
				ftp_usage();
			break;
		}
	}
}

int ftp_getcmd(ftp_t *ftp, char *s)
{
	char cmd[100];
	char p1[100];
	char p2[100];
	memset(cmd, 0 ,sizeof(cmd));
	memset(p1, 0 ,sizeof(p1));
	memset(p2, 0 ,sizeof(p2));
	int res;
	res = sscanf(s, "%s%s%s", cmd, p1, p2);
	if(res = 0)
		return -1;
	if(strcasecmp(cmd, "ls") == 0)
	{
		ftp->cmd = LS;
		if (*p1 != 0)
			strcpy(ftp->remotepath, p1);
		else
			strcpy(ftp->remotepath, "./");
		return 0;
	}

	if(strcasecmp(cmd, "download") == 0)
	{
		ftp->cmd = DOWNLOAD;
		if(*p1 != 0)
			strcpy(ftp->remotepath, p1);
		else
			return -1;
		char *name = strrchr(p1, '/');
		if(name == NULL)
			name = p1;

		if(*p2 != 0)
		{
			int len = strlen(p2);
			if(*(p2 + len -1) != '/')
				strcat(p2,"/");
		}
		else
			strcpy(p2, "./");
		sprintf(ftp->localpath, "%s%s", p2, name);
		return 0;
	}
	
	if(strcasecmp(cmd, "upload") ==0 )
	{
		ftp->cmd = UPLOAD;
		if(*p1 != 0)
			strcpy(ftp->localpath, p1);
		else
			return -1;
		char *name = strrchr(p1, '/');
		if(name == NULL)
			name = p1;

		if(*p2 != 0)
		{
			int len = strlen(p2);
			if(*(p2 + len -1) != '/')
				strcat(p2,"/");
		}
		else
			strcpy(p2, "./");
		sprintf(ftp->remotepath, "%s%s", p2, name);
		return 0;
	}

	if(strcasecmp(cmd, "q") == 0 || strcasecmp(cmd, "quit") == 0)
	{
		close(ftp->sockfd);
		printf("disconnect with FTP server\n");
		exit(0);
	}
	 
	return -1;
}

int main(int argc, char *argv[])
{
	char cmd[300];
	int res;
	ftp_t ftp;
	ftp_init(&ftp);	
	param_parse(&ftp, argc, argv);
	res = ftp_mkcon(&ftp);
		error(res, NULL, exit(-1));
	printf("login success\n");
	close(ftp.sockfd);
	error(res, login faild, exit(-1));
scan_input:
	while(1)
	{
		printf(">");
		gets(cmd);
		res = ftp_getcmd(&ftp, cmd);
		if(res == 0)
			break;
		printf("invalid cmd\n"
				"CMD:\n"
				"	ls		[remotepath]\n"
				"	download	<remotepath>	[localpath]\n"
				"	upload		<localpath>	[remotepath]\n");
	}
	res = ftp_mkcon(&ftp);
	switch(ftp.cmd)
	{
		case LS:
			ftp_list(&ftp);
		break;

		case DOWNLOAD:
			ftp_download(&ftp);
		break;

		case UPLOAD:
			ftp_upload(&ftp);
		break;

		default:
		break;
	}
	goto scan_input;
	return argc;
}
