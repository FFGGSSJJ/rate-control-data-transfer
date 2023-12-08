#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>

const int Gbit = 1024*1024*128;		// 1Gb in byte
const int Bms = 1024*128;				// to enable 1Gbps, min data size in byte needed per ms

/* receive data */
void recv_data(int sockfd){
	std::string filename = "testdata/recv.txt";
	std::ofstream recvfile(filename, std::ios::out);
	char buffer[Bms];

	/* receive data */
	int total_recv = 0;
	std::chrono::high_resolution_clock::time_point s0 = std::chrono::high_resolution_clock::now();
	while (true) {
		int ret = recv(sockfd, buffer, (size_t)Bms, 0);
		if (ret <= 0)
			break;
		
		/* check receive rate */
		total_recv += ret;
		if (total_recv >= Gbit) {
			std::chrono::high_resolution_clock::time_point e0 = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(e0 - s0).count();
			double durationms = (double) ((double)duration/(double)1000);
			double recv_rate = ((double)total_recv*8/1024/1024/1024)/(durationms/1000);
			s0 = e0;
			total_recv = 0;
			std::cout << "Recv_rate=" << recv_rate << "Gbs " << std::endl;
		}
		// bzero(buffer, Bms);
	}
	recvfile.close();
	return;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: ./server <ip_addr> <optional: receive_file_path>" << std::endl;
		return EXIT_FAILURE;
	}
	/* setup connection */
	std::string ip = argv[1];
	int port = 8080;

	struct sockaddr_in server_addr, new_addr;
	socklen_t addr_size;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("[-]Error in socket");
		exit(1);
	} printf("[+]Server socket created successfully.\n");

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(e < 0) {
		perror("[-]Error in bind");
		exit(1);
	} printf("[+]Binding successfull.\n");

	if(listen(sockfd, 10) == 0){
		printf("[+]Listening....\n");
	} else {
		perror("[-]Error in listening");
		exit(1);
	}

	addr_size = sizeof(new_addr);
	int new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);

	/* receive the data */
	recv_data(new_sock);
	printf("[+]Data receive successfully.\n");

	return 0;
}
