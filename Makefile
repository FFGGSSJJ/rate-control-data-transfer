server:
	g++ -Wall -pthread -g -std=c++11 server.cpp -o server

client:
	g++ -Wall -pthread -g -std=c++11 client.cpp -o client

clean:
	rm server
	rm client

all: server client
