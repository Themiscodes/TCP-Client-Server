#include "dataServer.h"
#include "helperFunctions.h"

// execution queue
queue<string> clients;

// mutex for the worker queue
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// condition variable if queue is empty
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

// condition variable if queue is full
pthread_cond_t full_pool = PTHREAD_COND_INITIALIZER;

// files left for each socket
map<int, int> unsatisfied_clients; 

// mutex for the map
pthread_mutex_t uc_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutexes for each socket
map<int, pthread_mutex_t> socketmap;

// mutex for socketmap
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

// these will be given as command line arguments
int thread_pool_size;
int queue_size;
int block_size;
int PORT;

int main(int argc, char **argv){

    // get the arguments with getopt
    if (argc != 9){
        std::cerr << "Wrong arguments.\nFormat: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n";
        exit(1);
    }
    int cc;
    while ((cc = getopt(argc, argv, "p:s:q:b:"))!=-1){
        switch(cc){
            case 'p':
                PORT = atoi(optarg);
                break;
            // worker threads size
            case 's':
                thread_pool_size = atoi(optarg);
                break;
            // queue size
            case 'q':
                queue_size = atoi(optarg);
                break;
            // block size
            case 'b':
                block_size = atoi(optarg);
                break;
            default:
                std::cerr << "Wrong arguments.\nFormat: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n";
                exit(1);
        }
    }

    // printing the parameters
    std::cout<< "\nServer's parameters are:\n";
    std::cout<< "port: "<< PORT <<"\n";
    std::cout<< "thread_pool_size: "<< thread_pool_size <<"\n";
    std::cout<< "queue_size: "<< queue_size <<"\n";
    std::cout<< "block_size: "<< block_size <<"\n";

    // thread pool 
    pthread_t thread_pool[thread_pool_size];
    for(int i=0;i< thread_pool_size;i++){

        // in worker_thread function they loop waiting for a job
        pthread_create(&thread_pool[i], NULL, worker_thread, NULL);
    }

    // AF_INET: internet socket, Protocol: 0, oti einai TCP
    int socketFD = errorcheck(socket(AF_INET, SOCK_STREAM, 0 ), "creating socket");

    // error handling is done internally
    bind_port(socketFD, PORT);
    std::cout << "Server was successfully initialised...\n";
    
    // listen for connections at socketFD
    errorcheck(listen(socketFD, BACKLOG), "listen");
    std::cout << "Listening for connections on port "<< PORT << "\n";

    while(1){

        // accept the connection
        struct sockaddr_in addr;
        socklen_t addr_len;
        int connectFD;
        errorcheck(connectFD = accept(socketFD, (SA*) &addr, &addr_len), "accept connection");            
        std::cout << "Accepted connection from " << inet_ntoa(addr.sin_addr) << "\n\n";

        int* clientSocket = new int;
        *clientSocket = connectFD;

        // first lock
        pthread_mutex_lock(&socket_mutex);

        // create a new mutex if there isn't one available
        if(socketmap.find(connectFD)==socketmap.end()){
            pthread_mutex_t new_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
            socketmap.insert(std::pair<int, pthread_mutex_t>(connectFD, new_socket_mutex));
        }
        pthread_mutex_unlock(&socket_mutex);

        // communication thread for the socket
        pthread_t com_thread;
        pthread_create(&com_thread, NULL, get_client_request, clientSocket);
        pthread_detach(com_thread);
        
    }
    
    return 0; 
}