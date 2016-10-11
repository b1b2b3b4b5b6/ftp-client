#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define sys_error(res,s,hander) do{\
	if(res == -1){perror(#s);hander;}\
}while(0)

#define error(res,s,func) do{\
	if(res == -1){\
		if(*(#s) != 0){\
			char *p = #s;\
			if(strpbrk(#s,"NULL" == 0));\
			else printf("%s\n",#s);}\
		func;}\
}while(0)

#define judge(exp) do{\
	(exp)?(printf("%s: true\n", #exp)):(printf("%s :false\n", #exp));\
}while(0)


#include <sys/types.h>

#include <sys/socket.h>

#include <sys/select.h>

#include <arpa/inet.h>

#include <unistd.h>

#include <pthread.h>

#include <stdlib.h>

#include <getopt.h>

#include <fcntl.h>


#endif
