#include "helperFunctions.h"

// error handler that prints a message and exits the process
int errorcheck(int i, const char * message){
    
    // if the return value is -1
    if (i ==-1){

        // print where the message was encountered and the Errno
        std::cerr << "Error Encountered with: " << message << std::endl;
        std::cerr << "Errno: " << errno << std::endl;
        _exit(1);
    }

    // else return the return value of the call
    return i;
}

// bind docket to an address
int bind_port(int socketFD, short port){

    // to pass the struct in bind
    struct sockaddr_in server;

    // address family is AF INET in the Internet domain
    server.sin_family = AF_INET;
    
    // address can be a specific IP or INADDR ANY (special address (0.0.0.0) meaning “any address”)
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // TCP port number 
    server.sin_port = htons(port);

    // binding socket socketFD to address 
    return errorcheck(bind(socketFD, (SA *) &server, sizeof(server)), "binding socket");
}

// handles the client request for the communication thread
void* get_client_request(void* conFD){
    int connectFD = *((int*)conFD);
    delete (int*)conFD;

    int buffer=4096;
    
    // to read and write
    size_t bytes_read;
    char buff[buffer];
    memset(buff, '\0', sizeof(char)*buffer);
    string dirpath = "";

    // read folder that was given
    while((bytes_read = read(connectFD, &buff, sizeof(char)*buffer))>0){
        dirpath +=buff;
        memset(buff, '\0', sizeof(char)*buffer);
        if (bytes_read < buffer) break;
    }

    // read error handling
    errorcheck(bytes_read, "read from socket");

    // confirmation message
    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: About to scan directory "<<  dirpath << std::endl;

    // how many files are needed to be processed
    count_files(dirpath, connectFD);

    // block size that server uses
    uint32_t server_block_size = htonl(block_size);
    write(connectFD, &server_block_size, sizeof(uint32_t));

    // pass the file count to the client
    int counter= unsatisfied_clients[connectFD];
    uint32_t count = htonl(counter);
    write(connectFD, &count, sizeof(uint32_t));

    // katalog recursively finds and adds the files in the queue
    katalog(dirpath, connectFD);

    return NULL;
}

// recursively counts the files in the folder and all subfolders
void count_files(string dirpath, int connectFD){
    
    // manipulate the string to use it as a directory
    if(dirpath.back()=='\0'){
        dirpath.pop_back();
    }

    if(dirpath.back()!='/'){
        dirpath+="/";
    }
    string pathie = dirpath;
    dirpath+='\0';

    char directory[dirpath.length()];
    strcpy(directory, dirpath.c_str());
    
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) {

        // for each directory and file
        while ((ent = readdir(dir)) != NULL) {
            string pathfile = pathie;
            pathfile += ent->d_name;
            pathfile += '\0';
            char pathname[pathfile.length()];
            strcpy(pathname, pathfile.c_str());
            
            // if it is not current or parent directory
            if (!( !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") )){
                struct stat pathINFO;
                stat(pathname, &pathINFO);
                if( S_ISREG(pathINFO.st_mode)){

                    pthread_mutex_lock(&uc_mutex);

                    // if it is not in the map add it
                    if(unsatisfied_clients.find(connectFD)==unsatisfied_clients.end()){
                        unsatisfied_clients.insert(std::pair<int, int>(connectFD, 1));
                    }
                    else{ // else increase the counter
                        unsatisfied_clients[connectFD]+=1;
                    }
                    pthread_mutex_unlock(&uc_mutex);

                }
                else if( S_ISDIR(pathINFO.st_mode)){
                    // if its a directory call recursively count_files
                    pathfile.pop_back();
                    count_files(pathfile, connectFD);
                }
            }
        }
        closedir(dir);
    } 
    else {
        // if it hasn't been found
        errorcheck(-1, "opening directory");
    }

}

// katalog recursively finds and adds the files in the queue
void katalog(string dirpath, int connectFD){

    // manipulate the string
    if(dirpath.back()=='\0'){
        dirpath.pop_back();
    }
    if(dirpath.back()!='/'){
        dirpath+="/";
    }

    string pathie = dirpath;
    dirpath+='\0';
    char directory[dirpath.length()];
    strcpy(directory, dirpath.c_str());
    
    // open the directory
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) {

        // for each folder
        while ((ent = readdir(dir)) != NULL) {
            string pathfile = pathie;
            pathfile += ent->d_name;
            pathfile += '\0';
            char pathname[pathfile.length()];
            strcpy(pathname, pathfile.c_str());
            
            //if it is not the parent or current directory
            if (!( !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") )){
                struct stat pathINFO;
                stat(pathname, &pathINFO);
                
                if( S_ISREG(pathINFO.st_mode)){

                    // add the socket and then the path file
                    string client_and_file = std::to_string(connectFD);
                    client_and_file += " ";
                    client_and_file += pathfile;
                    
                    pthread_mutex_lock(&pool_mutex);

                    // if the queue is full wait
                    while(clients.size()>=queue_size)
                        pthread_cond_wait(&full_pool, &pool_mutex);
                    
                    // print message of confirmation
                    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: Adding file "<< pathfile<<" to the queue..."<< std::endl;

                    // push the new work in the queue
                    clients.push(client_and_file);
                    pthread_cond_signal(&pool_cond);                   
                    pthread_mutex_unlock(&pool_mutex);

                }
                else if( S_ISDIR(pathINFO.st_mode)){

                    // remove the '\0'
                    pathfile.pop_back();
                    
                    // since it is a directory recursively call katalog
                    katalog(pathfile, connectFD);
                }
            }
        }
        closedir(dir);
    } 
    else {
        // error with open
        errorcheck(-1, "opening directory");
    }

}

// assigns a work from the queue to the worker thread
void* worker_thread(void* argi){

    while(1){

        // lock first
        pthread_mutex_lock(&pool_mutex);

        // if queue is empty wait on the cond
        while(clients.empty())
            pthread_cond_wait(&pool_cond, &pool_mutex);   
            
        // remove the work from the queue
        string clientrequest = clients.front();
        clients.pop();

        // print confirmation message
        std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: Received task: <"<<clientrequest<<">"<< std::endl;
        
        // signal waiting threads on full pool
        pthread_cond_signal(&full_pool);
        pthread_mutex_unlock(&pool_mutex);

        // call the function that sends the file and metadata to the client
        handle_client_request(clientrequest);
    }

    return NULL;

}

// function that sends the file and metadata to the client
void* handle_client_request(string client_request){

    // get socket FD from the string
    string CFD = "";
    int i=0;
    while (client_request[i]!=' '){
        CFD+=client_request[i];
        i++;
    }
    int connectFD = stoi(CFD);
    i++;

    // get the file path
    string file_in_question = "";
    while (client_request[i]!='\0'){
        file_in_question+=client_request[i];
        i++;
    }
    file_in_question+='\0';
    
    // initially check to find the socket mutex in the map
    pthread_mutex_lock(&socket_mutex);
    if(socketmap.find(connectFD)==socketmap.end()){
        errorcheck(-1, "mutex for socket unavailable");
    }
    pthread_mutex_unlock(&socket_mutex);

    // lock the mutex
    pthread_mutex_lock(&socketmap[connectFD]);

    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: About to read file "<<file_in_question << std::endl;

    // file to be sent
    char filepath[file_in_question.length()];
    strcpy(filepath, file_in_question.c_str());

    // open
    int FDR = errorcheck(open(filepath, O_RDONLY), "open");

    // send the name size
    uint32_t name_size = htonl(file_in_question.length());
    write(connectFD, &name_size, sizeof(uint32_t));
    
    // send the file path
    write(connectFD, &filepath, file_in_question.length());

    // to find the file size
    struct stat metadata;
    stat(filepath, &metadata);

    // uint32_t for Linux (adapt accordingly for different systems)
    uint32_t file_size = htonl(metadata.st_size);

    // send the file size
    write(connectFD, &file_size, sizeof(uint32_t));

    // send the file in block_size packets
    size_t bytes_read;
    char buff[block_size];
    memset(buff, '\0', sizeof(char)*block_size);
    while((bytes_read = read(FDR, &buff, block_size))>0){
        write(connectFD, buff, bytes_read);
        memset(buff, '\0', sizeof(char)*block_size);
    }

    // close the open file
    close(FDR);

    // lock the mutex for the map
    pthread_mutex_lock(&uc_mutex);

    // error handling if its not found
    if(unsatisfied_clients.find(connectFD)==unsatisfied_clients.end()){
        errorcheck(-1, "map unsatisfied_clients doesn't have this key.");
    }
    else if(unsatisfied_clients[connectFD]>1){ // if it wasnt the last reduce the counter
        unsatisfied_clients[connectFD]-=1;
    }
    else { // if all files have been sent close the socket
        unsatisfied_clients.erase(unsatisfied_clients.find(connectFD));
        close(connectFD);
    }
    pthread_mutex_unlock(&uc_mutex);

    // unlock the socket mutex
    pthread_mutex_unlock(&socketmap[connectFD]);

    return NULL;

}