#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <thread>
#include <dirent.h>
#include <libgen.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/ioctl.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// error handler
int errorcheck(int i, const char * message);

// for ease of use
#define SA struct sockaddr