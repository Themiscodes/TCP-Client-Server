#include "dataServer.h"

// error handler
int errorcheck(int i, const char * message);

// binding function
int bind_port(int socketFD, short port);

// worker thread function to send a file
void* handle_client_request(string client_request);

// handles socket request
void* get_client_request(void* conFD);

// worker thread function that awaits assignments
void* worker_thread(void* argi);

// adds all the dirpath files recursively in the queue
void katalog(string dirpath, int connectFD);

// similar function that adds the number of files in the map
void count_files(string dirpath, int connectFD);