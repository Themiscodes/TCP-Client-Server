# TCP Client Server

My implementation of a multithreaded TCP Client/Server written in C++. The purpose of this project was to grasp the concepts of socket programming, utilize the pthread library to manage threads and synchronise them using mutexes and condition variables.

TCP is a reliable protocol that guarantees that the data remain intact and arrive in the same order in which they were sent. While, multithreading allows multiple clients to connect and interact with the server in tandem, without significant drop-offs in transmission time.

## Implementation

- The Client connects and requests a specific directory from the Server. Then, it receives each file along with its information, so that it can locally replicate the same folder hierarchy. 

- The Server creates two kinds of threads to handle the requests. The **communication threads** that are responsible for each client and the **worker threads** that send the respective files.

![ServerClient](https://user-images.githubusercontent.com/73662635/180067338-e6df7da1-c5e4-4f0c-89e2-a787c2d01608.png)

## Data Server

I chose a rather simple approach of keeping the thread functions in the [helperFunctions.cpp](Data_Server/helperFunctions.cpp) file and the basic server structure in [dataServer.cpp](Data_Server/dataServer.cpp).

- On startup, the server awaits for connections from clients on the predefined port that is provided as an argument.

- When a client sends a request, a new communication thread is being created to handle it. This way the server is able to receive and process requests by multiple clients at a time.

- The directory is read from the server's local file system recursively, until each file in the hierarchy has been placed in the worker queue.

- When a worker thread is available, it gets allocated a file transfer job, if the queue is not empty. Then, it processes that file and sends it to the client.

- Finally, with the use of a map of mutexes for each socket, I ensure that only one worker thread at a time can write on a specific socket.

## Remote Client
The code for the client is in the [remoteClient.cpp](Remote_Client/remoteClient.cpp) file.
- On the client side the server IP, port number and directory that is requested from the server is given as an argument. 
- A socket is created to connect to the server and the request is sent. 
- Then, for each file of the directory it reads from the socket the file path, metadata and the file itself. Using this information it replicates locally the same folder structure.
- If a file already exists locally, then it gets deleted and replaced with the new file sent from the server.

## Compilation and Execution

To compile the necessary files for the Server:
```
$ make server
```

To compile the necessary files for the Client:
```
$ make client
```

To compile all the files:
```
$ make all
```

To start the server in the [Data_Server](Data_Server/) directory run:
```
./dataServer -p <port> -s <pool_size> -q <queue_size> -b <block_size>
```

To start the client in the [Remote_Client](Remote_Client/) directory run:
```
./remoteClient -i <server_ip> -p <server_port> -d <directory>
```

To delete all the executable and object files generated:
```
$ make clean
```
