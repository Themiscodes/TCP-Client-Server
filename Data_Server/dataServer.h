#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iterator>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <queue>
#include <thread>
#include <dirent.h>
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// backlog size
#define BACKLOG 1280

// for ease of use
#define SA struct sockaddr

using std::queue;
using std::string;
using std::map;

// execution queue
extern queue<string> clients; 

// map of requests for each socket
extern map<int, int> unsatisfied_clients; 

// mutex for the map
extern pthread_mutex_t uc_mutex;

// mutex for the thread_pool
extern pthread_mutex_t pool_mutex;

// condition variable if thread_pool is empty
extern pthread_cond_t pool_cond;

// condition variable if thread_pool is full
extern pthread_cond_t full_pool;

// so that helperfunctions.cpp can access these variable
extern int block_size;
extern int thread_pool_size;
extern int queue_size;

// map socket and mutex
extern map<int, pthread_mutex_t> socketmap;

// mutex for this map
extern pthread_mutex_t socket_mutex;