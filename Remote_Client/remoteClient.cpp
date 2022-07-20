#include "remoteClient.h"

int main(int argc, char *argv[]){

    // arguments given from the command line
    int port, sockFD;     
    struct hostent *server_ip;
    char *directory;

    // get the arguments with getopt
    if (argc != 7){
        std::cerr << "Wrong arguments.\nFormat: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n";
        exit(1);
    }
    int cc;
    while ((cc = getopt(argc, argv, "i:p:d:"))!=-1){
        switch(cc){
            // server ip
            case 'i':
                if ((server_ip = gethostbyname(optarg)) == NULL){
                    herror(" gethostbyname ");
                    exit(1);
                }
                break;
            // port
            case 'p':
                port = atoi(optarg);
                break;
            // directory
            case 'd':
                directory = optarg;
                break;
            default:
                std::cerr << "Wrong arguments.\nFormat: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n";
                exit(1);
        }
    }

    // print the parameters to be used
    std::cout<< "\nClient's parameters are:\n";
    std::cout<< "serverIP: "<< server_ip->h_name <<"\n";
    std::cout<< "port: "<< port <<"\n";
    std::cout<< "directory: "<< directory <<"\n";

    struct sockaddr_in server;

    // creating the socket
    errorcheck(sockFD = socket(AF_INET, SOCK_STREAM, 0), "socket");

    // AF INET for Internet domain
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, server_ip->h_addr, server_ip->h_length);

    // TCP port number 
    server.sin_port = htons(port);

    // connect to the server
    errorcheck(connect(sockFD, (SA *) &server, sizeof(server)), "connect");

    // print confirmation message
    std::cout << "Established Connection to: " << inet_ntoa(server.sin_addr) << " on port: " << port << "\n\n";
    
    // directory to fetch
    char folder[strlen(directory)+1];
    for(int i=0;i<strlen(directory);i++){
        folder[i]=directory[i];
    }
    folder[strlen(directory)]='\0';

    // write directory name in the socket
    errorcheck(write(sockFD, folder, strlen(directory)+1), "write path");

    // initially read block size
    size_t bytes_read = 0;
    uint32_t server_block_size;
    bytes_read = read(sockFD, &server_block_size, sizeof(uint32_t));
    if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");

    // so that there is consistency
    uint32_t block_size = ntohl(server_block_size);

    // files to be recieved
    uint32_t folder_file_count;
    bytes_read = read(sockFD, &folder_file_count, sizeof(uint32_t));
    if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
    uint32_t file_count = ntohl(folder_file_count);

    char buff[block_size];
    // for each file
    for(int i =0 ; i<file_count;i++){

        // read file size
        uint32_t name_size;
        bytes_read = read(sockFD, &name_size, sizeof(uint32_t));
        if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
        uint32_t filepath_size = ntohl(name_size);

        // read the file path
        bytes_read = read(sockFD, &buff, filepath_size);
        if(bytes_read!=filepath_size) errorcheck(-1, "read");

        char filename[filepath_size];
        strncpy(filename, buff, filepath_size);

        // check if the directories are already present
        char *directory = dirname(buff);
        struct stat dirStat;
        if(!(stat(directory, &dirStat)==0 && S_ISDIR(dirStat.st_mode))){

            // if not create those that don't exist
            char st[strlen(directory)+10];
            sprintf(st, "mkdir -p %s", directory);
            system(st);
        }

        // check if the file exists
        struct stat fileStat;
        if(stat(buff, &fileStat)==0 && S_ISREG(fileStat.st_mode)){
            
            // if yes delete it first
            char st[filepath_size+4];
            sprintf(st, "rm %s", buff);
            system(st);
        }

        // create the file
        int fd = errorcheck(open(filename, O_WRONLY | O_CREAT, 0644), "open");

        // get the size from the socket
        uint32_t file_sizeH;
        bytes_read = read(sockFD, &file_sizeH, sizeof(uint32_t));
        if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
        size_t file_size = ntohl(file_sizeH);
        
        // clean the buffer
        memset(buff, '\0', sizeof(char)*block_size);
        
        // till all the bytes have been read
        while(file_size !=0 ){

            // get block size bytes if there are more left
            if(file_size>block_size){ 
                bytes_read = read(sockFD, &buff, block_size);
                if(bytes_read!=block_size) errorcheck(-1, "read");
                file_size-=bytes_read;
            }
            else{ 
                // else what is left
                bytes_read = read(sockFD, &buff, file_size);
                if(bytes_read!=file_size) errorcheck(-1, "read");
                file_size-=bytes_read;
            }

            // write what was read
            int dd = write(fd, buff, bytes_read);

            // check that write worked
            if (dd != bytes_read) 
                errorcheck(-1, "write to file");

            // clean the buffer
            memset(buff, '\0', sizeof(char)*block_size);
        }
        
         // close the file
        close(fd);

        // print confirmation message
        std::cout << "Received: " << filename << "\n";
    }
    close(sockFD);
}

// error handler 
int errorcheck(int i, const char * message){
    
    // if return value was -1
    if (i ==-1){

        // print error message and exit
        std::cerr << "Error Encountered with: " << message << std::endl;
        std::cerr << "Errno: " << errno << std::endl;
        _exit(1);
    }

    // else return the return value 
    return i;
}