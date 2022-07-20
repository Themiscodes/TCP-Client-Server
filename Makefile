all: server client

server: Data_Server/helperFunctions.o Data_Server/dataServer.o
	g++ -o Data_Server/dataServer Data_Server/dataServer.o Data_Server/helperFunctions.o -pthread

client: Remote_Client/remoteClient.o
	g++ -o Remote_Client/remoteClient Remote_Client/remoteClient.o -pthread

Remote_Client/remoteClient.o: Remote_Client/remoteClient.cpp Remote_Client/remoteClient.h
	g++ -c Remote_Client/remoteClient.cpp -o Remote_Client/remoteClient.o -pthread

Data_Server/helperFunctions.o: Data_Server/helperFunctions.cpp Data_Server/helperFunctions.h Data_Server/dataServer.h
	g++ -c Data_Server/helperFunctions.cpp -o Data_Server/helperFunctions.o -pthread

Data_Server/dataServer.o: Data_Server/dataServer.cpp Data_Server/dataServer.h Data_Server/helperFunctions.h
	g++ -c Data_Server/dataServer.cpp -o Data_Server/dataServer.o -pthread

clean:
	rm -f Remote_Client/remoteClient Data_Server/dataServer Remote_Client/*.o Data_Server/*.o